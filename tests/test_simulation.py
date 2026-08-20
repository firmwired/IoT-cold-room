import unittest

from simulation import ColdRoomController, ControllerConfig, run_simulation


class TestColdRoomController(unittest.TestCase):
    def setUp(self) -> None:
        self.controller = ColdRoomController(ControllerConfig())

    def test_turns_cooling_on_when_temperature_is_above_max(self) -> None:
        command = self.controller.control(9.0)
        self.assertEqual(command, "COOLING_ON")
        self.assertTrue(self.controller.compressor_on)

    def test_turns_cooling_off_when_temperature_is_below_min(self) -> None:
        self.controller.compressor_on = True
        command = self.controller.control(1.0)
        self.assertEqual(command, "COOLING_OFF")
        self.assertFalse(self.controller.compressor_on)

    def test_emits_critical_alerts_outside_critical_bounds(self) -> None:
        self.assertTrue(self.controller.alerts(-1.0))
        self.assertTrue(self.controller.alerts(11.0))
        self.assertFalse(self.controller.alerts(5.0))

    def test_simulation_moves_towards_operating_range(self) -> None:
        results = run_simulation(duration_minutes=10, initial_temp_c=12.0)
        self.assertEqual(results[0].command, "COOLING_ON")
        self.assertLess(results[-1].temperature_c, results[0].temperature_c)


if __name__ == "__main__":
    unittest.main()
