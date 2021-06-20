ASETEK_PRO = [
    # Read Pump RPM
    {
        "input": r"(31)",
        "output": r"\g<1>12341122"
    },
    # Read Fan RPM
    {
        "input": r"(41)([0-9a-f]{2})",
        "output": r"\g<1>1234\g<2>1122"
    },
    # Read Temperature
    {
        "input": r"(a9)",
        "output": r"\g<1>12342005"
    },
    # Read Firmware Version
    {
        "input": r"(aa)",
        "output": r"\g<1>1234010203ff"
    },
    # Write Pump Mode
    {
        "input": r"(32)0[0-2]",
        "output": r"\g<1>1234"
    },
    # Write Fan PWM
    {
        "input": r"(42)([0-9a-f]{2})[0-9a-f]{2}",
        "output": r"\g<1>1234"
    },
    # Write Fan RPM
    {
        "input": r"(43)([0-9a-f]{2})[0-9a-f]{4}",
        "output": r"\g<1>1234"
    },
    # Write Fan Curve
    {
        "input": r"(40)([0-9a-f]{2})[0-9a-f]{28}",
        "output": r"\g<1>1234"
    }
]
