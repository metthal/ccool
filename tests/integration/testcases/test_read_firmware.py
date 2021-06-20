from framework import Call, Repeats, Sequence


def test_read_firmware(fakedev, ccool):
    assert ccool.run("firmware") == {"version": {"major": 1, "minor": 2, "patch": 3}}, "Read Firmware did not receive correct response"

    fakedev.assert_has_message_pattern(
        Repeats(
            Sequence(
                Call("send", endpoint=1, data="aa"),
                Call("recv", endpoint=1)
            ),
            min=1,
            max=1
        ),
        title="Read Temperature"
    )
