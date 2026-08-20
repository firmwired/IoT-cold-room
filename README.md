# IoT-cold-room

Étude et simulation d'un système de surveillance et de commande d'une chambre froide.

## Objectif

Ce dépôt propose une simulation simple d'un système IoT de chambre froide :
- **Surveillance** de la température et génération d'alertes
- **Commande** d'un compresseur pour maintenir la consigne thermique

## Exécution

```bash
python3 /home/runner/work/IoT-cold-room/IoT-cold-room/simulation.py
```

## Tests

```bash
python3 -m unittest discover -s /home/runner/work/IoT-cold-room/IoT-cold-room/tests -p "test_*.py"
```