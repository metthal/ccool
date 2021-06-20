from framework import Call, Repeats, Sequence


def test_write_fan_pwm(fakedev, ccool):
    assert ccool.run("fans", "pwm", 42) == {}, "Write Fan PWM did not receive correct response"

    for i in range(fakedev.spec["fans"]):
        fakedev.assert_has_message_pattern(
            Repeats(
                Sequence(
                    Call("send", endpoint=1, data=f"42{i:02x}2a"),
                    Call("recv", endpoint=1)
                ),
                min=1,
                max=1
            ),
            title=f"Write Fan PWM #{i}"
        )
