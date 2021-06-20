from framework import Call, Repeats, Sequence


def test_read_fan_rpm(fakedev, ccool):
    assert ccool.run("fans") == {"rpm": [0x1122] * fakedev.spec["fans"]}, "Read Fan RPM did not receive correct response"

    for i in range(fakedev.spec["fans"]):
        fakedev.assert_has_message_pattern(
            Repeats(
                Sequence(
                    Call("send", endpoint=1, data=f"41{i:02}"),
                    Call("recv", endpoint=1)
                ),
                min=1,
                max=1
            ),
            title=f"Read Fan RPM #{i}"
        )
