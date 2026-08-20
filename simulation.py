from dataclasses import dataclass
from typing import List


@dataclass
class ControllerConfig:
    min_temp_c: float = 2.0
    max_temp_c: float = 8.0
    critical_min_temp_c: float = 0.0
    critical_max_temp_c: float = 10.0
    cooling_delta_c: float = 0.8
    passive_warming_factor: float = 0.15
    ambient_temp_c: float = 22.0


@dataclass
class StepResult:
    minute: int
    temperature_c: float
    compressor_on: bool
    command: str
    alerts: List[str]


class ColdRoomController:
    def __init__(self, config: ControllerConfig | None = None) -> None:
        self.config = config or ControllerConfig()
        self.compressor_on = False

    def control(self, temperature_c: float) -> str:
        if temperature_c > self.config.max_temp_c:
            self.compressor_on = True
            return "COOLING_ON"
        if temperature_c < self.config.min_temp_c:
            self.compressor_on = False
            return "COOLING_OFF"
        return "HOLD"

    def alerts(self, temperature_c: float) -> List[str]:
        issues: List[str] = []
        if temperature_c < self.config.critical_min_temp_c:
            issues.append("ALERTE_CRITIQUE: température trop basse")
        if temperature_c > self.config.critical_max_temp_c:
            issues.append("ALERTE_CRITIQUE: température trop élevée")
        return issues

    def simulate_step(self, minute: int, temperature_c: float) -> StepResult:
        command = self.control(temperature_c)
        alert_list = self.alerts(temperature_c)
        return StepResult(
            minute=minute,
            temperature_c=round(temperature_c, 2),
            compressor_on=self.compressor_on,
            command=command,
            alerts=alert_list,
        )

    def evolve_temperature(self, temperature_c: float) -> float:
        if self.compressor_on:
            return temperature_c - self.config.cooling_delta_c
        return temperature_c + (self.config.ambient_temp_c - temperature_c) * self.config.passive_warming_factor


def run_simulation(
    duration_minutes: int = 30,
    initial_temp_c: float = 12.0,
    config: ControllerConfig | None = None,
) -> List[StepResult]:
    controller = ColdRoomController(config=config)
    temp = initial_temp_c
    timeline: List[StepResult] = []

    for minute in range(duration_minutes):
        step = controller.simulate_step(minute=minute, temperature_c=temp)
        timeline.append(step)
        temp = controller.evolve_temperature(temp)

    return timeline


if __name__ == "__main__":
    for entry in run_simulation():
        alert_suffix = f" | {'; '.join(entry.alerts)}" if entry.alerts else ""
        print(
            f"t={entry.minute:02d} min | T={entry.temperature_c:05.2f}°C | "
            f"commande={entry.command} | compresseur={'ON' if entry.compressor_on else 'OFF'}{alert_suffix}"
        )
