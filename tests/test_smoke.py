from trumptailer import core


def test_core_module_imports_and_runs():
    assert core.add(2, 3) == 5
    assert core.__version__ == "0.1.0"
