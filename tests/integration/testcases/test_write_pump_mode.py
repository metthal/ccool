from framework import Call, Repeats, Sequence


def test_write_pump_mode(fakedev, ccool):
    assert ccool.run("pump", 2) == {}, "Write Pump Mode did not receive correct response"

    fakedev.assert_has_message_pattern(
        Repeats(
            Sequence(
                Call("send", endpoint=1, data="3202"),
                Call("recv", endpoint=1)
            ),
            min=1,
            max=1
        ),
        title="Write Pump Mode"
    )
