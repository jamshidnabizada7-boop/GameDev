"""Run the death and restart regression with reduced motion enabled in memory."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name("test_restart.py")), init_globals={"REDUCED_MOTION_TEST": True})
