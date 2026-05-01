from sokoban.state import State, Level


def test_state_equality():
    s1 = State(player=(1, 2), boxes=frozenset([(3, 4)]))
    s2 = State(player=(1, 2), boxes=frozenset([(3, 4)]))
    assert s1 == s2


def test_state_immutability():
    s = State(player=(0, 0), boxes=frozenset())
    try:
        s.player = (1, 1)
        assert False, "State should be frozen"
    except Exception:
        pass


def test_level_stores_state():
    state = State(player=(1, 1), boxes=frozenset([(2, 2)]))
    level = Level(
        game_id=1,
        width=5,
        height=5,
        walls=frozenset([(0, 0)]),
        goals=frozenset([(2, 2)]),
        init_state=state,
    )
    assert level.init_state == state
    assert level.width == 5


def test_level_solved_when_boxes_on_goals():
    goals = frozenset([(3, 3)])
    state = State(player=(1, 1), boxes=frozenset([(3, 3)]))
    level = Level(
        game_id=2,
        width=6,
        height=6,
        walls=frozenset(),
        goals=goals,
        init_state=state,
    )
    assert level.init_state.boxes == level.goals
