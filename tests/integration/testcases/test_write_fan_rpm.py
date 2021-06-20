from framework import Call, Repeats, Sequence


def test_write_fan_rpm(fakedev, ccool):
    assert ccool.run("fans", "rpm", 1234) == {}, "Write Fan RPM did not receive correct response"

    for i in range(fakedev.spec["fans"]):
        fakedev.assert_has_message_pattern(
            Repeats(
                Sequence(
                    Call("send", endpoint=1, data=f"43{i:02x}04d2"),
                    Call("recv", endpoint=1)
                ),
                min=1,
                max=1
            ),
            title=f"Write Fan RPM #{i}"
        )
