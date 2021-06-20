__all__ = [
    "main"
]

import os
import re
import sys
import textwrap
import yaml

from pathlib import Path


TYPE_RE = re.compile(r"(u|i)(8|16|32|64)")
RETURN_TYPES = {
    "read_fan_rpm": "std::tuple<std::uint32_t, std::uint16_t>",
    "read_firmware_version": "std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>",
    "read_pump_rpm": "std::uint16_t",
    "read_temperature": "FixedPoint<16>"
}
PROTOCOL_ID_TO_NAME = {}


def snake_case_to_camel_case(name: str) -> str:
    return "".join([part.title() for part in name.split("_")])


def object_name_to_type_name(name: str):
    return name.replace(" ", "")


def spec_endian_to_cpp_endian(endian: str):
    return "Endian::Big" if endian == "big" else "Endian::Little"


def spec_type_to_cpp_type(type_name: str, allow_ref=False):
    end_pos = type_name.find("[")
    if end_pos != -1:
        is_array = True
    else:
        is_array = False
        end_pos = len(type_name)
    base_type = type_name[:end_pos]

    if base_type == "fx16":
        result = "FixedPoint<16>"
        if allow_ref:
            result = f"const {result}&"
    else:
        matches = TYPE_RE.fullmatch(base_type)
        if matches is not None:
            result = "std::{}int{}_t".format("u" if matches.group(1) == "u" else "", matches.group(2))

    if is_array:
        array_size = int(type_name[end_pos+1:-1])
        result = f"std::vector<{result}>"
        if allow_ref:
            result = f"const {result}&"

    return result


def spec_type_size(type_name: str):
    end_pos = type_name.find("[")
    if end_pos != -1:
        is_array = True
    else:
        is_array = False
        end_pos = len(type_name)
    base_type = type_name[:end_pos]

    if base_type == "fx16":
        result = 2
    else:
        matches = TYPE_RE.fullmatch(base_type)
        if matches is not None:
            result = int(matches.group(2)) >> 3

    if is_array:
        array_size = int(type_name[end_pos+1:-1])
        result = result * array_size

    return result


def spec_type_is_array(type_name: str):
    return type_name.find("[") != -1


def arg_to_cpp_arg(arg: dict):
    return "{} {}".format(
        spec_type_to_cpp_type(arg["type"], allow_ref=True),
        arg["name"]
    )


def arg_precondition(request_name: str, arg: dict):
    if spec_type_is_array(arg["type"]):
        end_pos = arg["type"].find("[")
        array_size = int(arg["type"][end_pos+1:-1])
        return """\tif ({arg_name}.size() != {array_size})
\t{{
\t\tspdlog::error("{request_name}: Argument '{arg_name}' has size {{}} while {array_size} is expected", {arg_name}.size());
\t\tthrow std::runtime_error("{request_name}: '{arg_name}' needs to have size {array_size}");
\t}}""".format(
    arg_name=arg["name"],
    array_size=array_size,
    request_name=request_name
)


def to_buffer_write(arg):
    return "buffer.write<endian, {}>({});".format(spec_type_to_cpp_type(arg["type"]), arg["name"])


def to_buffer_read(attr, create_var):
    return "{}response.read<endian, {}>().value();".format("auto result_{} = ".format(attr["name"]) if create_var else "", spec_type_to_cpp_type(attr["type"]))


def message_to_method_declaration(message):
    request = message["request"] or []
    response = message["response"] or []
    args = ["std::uint8_t endpoint"] + [arg_to_cpp_arg(arg) for arg in request]
    preconditions = list(filter(lambda pc: pc is not None, [arg_precondition(message["name"], arg) for arg in request]))
    buffer_writes = [to_buffer_write(arg) for arg in request]
    buffer_reads = [to_buffer_read(attr, attr["name"] in message.get("returns", [])) for attr in response]
    response_size = 1 + sum([spec_type_size(attr["type"]) for attr in response])
    response_type = RETURN_TYPES.get(message["name"], "void")
    return_values = ["result_{}".format(attr) for attr in message.get("returns", [])]
    if len(return_values) == 0:
        return_values = ""
    elif len(return_values) == 1:
        return_values = return_values[0]
    else:
        return_values = "{{{}}}".format(", ".join(return_values))
    return """virtual {response_type} {request_name}({args}) override
{{
{preconditions}

\tBuffer buffer;
\tbuffer.write<endian, OpcodeType>({opcode:#04x});
{buffer_writes}

\tauto response = send(endpoint, buffer);
\tif (response.get_size() != {response_size})
\t{{
\t\tspdlog::error("{request_name}: Obtained response with unexpected size '{{}}' while '{response_size}' was expected", response.get_size());
\t\tthrow std::runtime_error("Invalid response");
\t}}

\tauto opcode = response.read<endian, OpcodeType>().value();
\tif (opcode != {opcode:#04x})
\t{{
\t\tspdlog::error("{request_name}: Obtained response with opcode '{{:#04x}}' while '{opcode:#04x}' was expected", opcode);
\t\tthrow std::runtime_error("Invalid response");
\t}}

{buffer_reads}
\treturn {return_values};
}}
""".format(
        response_type=response_type,
        request_name=message["name"],
        args=", ".join(args),
        preconditions="\n\n".join(preconditions) or "",
        buffer_writes=textwrap.indent("\n".join(buffer_writes), "\t"),
        opcode=message["opcode"],
        response_size=response_size,
        buffer_reads=textwrap.indent("\n".join(buffer_reads), "\t"),
        return_values=return_values
    )


def device_spec_to_cpp_class(device_spec: dict):
    return """#pragma once

#include <device.hpp>
#include <protocols/{protocol_name}.hpp>

namespace ccool {{

class {class_name} : public Device<{protocol_type_name}>
{{
public:
\t{class_name}(std::unique_ptr<DeviceInterface>&& device_interface) : Device(std::move(device_interface), "{device_name}", {fan_count}, {endpoint}) {{}}
\tvirtual ~{class_name}() = default;
}};

}} // namespace ccool""".format(
        protocol_name=device_spec["protocol"],
        class_name=object_name_to_type_name(device_spec["name"]),
        device_name=device_spec["name"],
        protocol_type_name=object_name_to_type_name(PROTOCOL_ID_TO_NAME[device_spec["protocol"]]),
        fan_count=device_spec["fans"],
        endpoint=device_spec["usb"]["endpoint"]
    )


def protocol_spec_to_cpp_class(protocol_spec: dict):
    messages = [message_to_method_declaration(msg) for msg in protocol_spec["messages"]]
    pre_request = ["_device_interface->{}({});".format(action["method"], ", ".join([f"{arg:#04x}" for arg in action["args"]])) for action in protocol_spec["pre_request"]]
    post_response = ["_device_interface->{}({});".format(action["method"], ", ".join([f"{arg:#04x}" for arg in action["args"]])) for action in protocol_spec["post_response"]]
    return """#pragma once

#include <spdlog/spdlog.h>

#include <endian.hpp>
#include <protocol.hpp>

namespace ccool {{

class {class_name} : public Protocol<{endian}, {opcode_type}>
{{
public:
\t{class_name}(DeviceInterface* device_interface) : Protocol(device_interface, "{protocol_name}") {{}}
\tvirtual ~{class_name}() = default;

\tvirtual void pre_request() override
\t{{
{pre_request}
\t}}

\tvirtual void post_response() override
\t{{
{post_response}
\t}}

{messages}
}};

}} // namespace ccool""".format(
        class_name=object_name_to_type_name(protocol_spec["name"]),
        protocol_name=protocol_spec["name"],
        endian=spec_endian_to_cpp_endian(protocol_spec["endian"]),
        opcode_type=spec_type_to_cpp_type(protocol_spec["opcode"]),
        messages=textwrap.indent("\n".join(messages), "\t"),
        pre_request=textwrap.indent("\n".join(pre_request), "\t\t"),
        post_response=textwrap.indent("\n".join(post_response), "\t\t")
    )


def generate_all_devices(device_specs):
    conditions=["if (vendor_id == {:#06x} && product_id == {:#06x})\n{{\n\treturn std::make_unique<{}>(std::move(device_interface));\n}}".format(device_spec["usb"]["vendor_id"], device_spec["usb"]["product_id"], object_name_to_type_name(device_spec["name"])) for _, device_spec in device_specs]
    return """#include <devices/all.hpp>

{includes}

namespace ccool {{

std::unique_ptr<BaseDevice> check_known_devices(std::uint32_t vendor_id, std::uint32_t product_id, std::unique_ptr<DeviceInterface>&& device_interface)
{{
{conditions}

\treturn nullptr;
}}

}} // namespace ccool""".format(
        includes="\n".join([f"#include <devices/{spec_name}.hpp>" for spec_name, _ in device_specs]),
        conditions=textwrap.indent("\nelse ".join(conditions), "\t")
    )


def yamls_in_dir(dir_path: Path):
    for root, dirs, files in os.walk(dir_path):
        for f in files:
            if f.endswith(".yaml") or f.endswith(".yml"):
                yield Path(f).name.split('.', 1)[0], Path(root) / f


def load_specs(spec_file_paths):
    specs = []
    for spec_name, spec_file in spec_file_paths:
        with open(spec_file, "r") as f:
            spec = yaml.load(f, Loader=yaml.SafeLoader)
            PROTOCOL_ID_TO_NAME[spec["id"]] = spec["name"]
            specs.append((spec_name, spec))
    return specs


def main():
    devices_dir = Path(sys.argv[1]) / "devices"
    protocols_dir = Path(sys.argv[1]) / "protocols"

    devices_dest_dir = Path(sys.argv[2]) / "devices"
    protocols_dest_dir = Path(sys.argv[2]) / "protocols"

    Path(devices_dest_dir).mkdir(exist_ok=True)
    Path(protocols_dest_dir).mkdir(exist_ok=True)

    device_spec_files = yamls_in_dir(devices_dir)
    protocol_spec_files = yamls_in_dir(protocols_dir)

    device_specs = load_specs(device_spec_files)
    protocol_specs = load_specs(protocol_spec_files)

    for spec_name, device_spec in device_specs:
        with open(devices_dest_dir / f"{spec_name}.hpp", "w+") as f:
            f.write(device_spec_to_cpp_class(device_spec))

    with open(devices_dest_dir / f"all.cpp", "w+") as f:
        f.write(generate_all_devices(device_specs))

    for spec_name, protocol_spec in protocol_specs:
        with open(protocols_dest_dir / f"{spec_name}.hpp", "w+") as f:
            f.write(protocol_spec_to_cpp_class(protocol_spec))
