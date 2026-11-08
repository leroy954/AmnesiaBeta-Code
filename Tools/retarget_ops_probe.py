"""Runs inside the editor (python editor_py.py retarget_ops_probe.py <retargeter path>). Lists the ops of a retargeter and their settings."""
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    path = sys.argv[1] if sys.argv[1].startswith("/") else "/" + sys.argv[1]
    ctrl = unreal.IKRetargeterController.get_controller(unreal.load_asset(path))

    for i in range(ctrl.get_num_retarget_ops()):
        op = ctrl.get_op_controller(i)
        log("==", i, ctrl.get_op_name(i), type(op).__name__, [n for n in dir(op) if not n.startswith("_") and "settings" in n.lower()])

        for getter in ("get_settings",):
            if hasattr(op, getter):
                s = getattr(op, getter)()
                log("   ", type(s).__name__)

                import warnings
                warnings.simplefilter("ignore")

                for prop in [n for n in dir(s) if not n.startswith("_")]:
                    try:
                        log("      ", prop, "=", s.get_editor_property(prop))
                    except Exception:
                        pass
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
