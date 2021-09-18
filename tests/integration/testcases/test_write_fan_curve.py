from framework import Call, Repeats, Sequence


def test_write_fan_curve(fakedev, ccool):
    assert ccool.run("fans", "curve", "25-0", "35-30", "40-40", "45-50", "50-60", "55-75", "60-100") == {}, "Write Fan Curve did not receive correct response"

    for i in range(fakedev.spec["fans"]):
        fakedev.assert_has_message_pattern(
            Repeats(
                Sequence(
                    Call("send", endpoint=1, data=f"40{i:02x}1923282d32373c001e28323c4b64"),
                    Call("recv", endpoint=1)
                ),
                min=1,
                max=1
            ),
            title=f"Write Fan Curve #{i}"
        )
