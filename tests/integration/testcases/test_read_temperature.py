from framework import Call, Repeats, Sequence


def test_read_temperature(fakedev, ccool):
    assert ccool.run("temp") == {"temperature": 32.5}, "Read Temperature did not receive correct response"

    fakedev.assert_has_message_pattern(
        Repeats(
            Sequence(
                Call("send", endpoint=1, data="a9"),
                Call("recv", endpoint=1)
            ),
            min=1,
            max=1
        ),
        title="Read Temperature"
    )
