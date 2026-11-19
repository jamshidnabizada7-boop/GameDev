"""Observe a fresh arena for ten minutes of wall and game time."""
import unreal as ue
import json
import time
import os
world = ue.EditorLevelLibrary.get_pie_worlds(False)[0]
npcs = ue.GameplayStatics.get_all_actors_of_class(world, ue.FoundationNPC)
start = time.monotonic()
game_start = ue.GameplayStatics.get_time_seconds(world)
samples = []
root = 'C:/GameDev/AfghanistanFPS/Saved/Phase5'
os.makedirs(root, exist_ok=True)

def sample_soak(_):
    wall = time.monotonic() - start
    game = ue.GameplayStatics.get_time_seconds(world) - game_start
    complete = wall >= 600 and game >= 600
    if not samples or wall-samples[-1]['wall_seconds'] >= 30 or complete:
        samples.append(dict(wall_seconds=wall, game_seconds=game,
            delta_seconds=ue.GameplayStatics.get_world_delta_seconds(world),
            actors=[dict(name=n.get_actor_label(), arrivals=n.patrol_arrivals, steps=n.footsteps, health=n.health_value(), state=str(n.state)) for n in npcs]))
        with open(root+'/soak.json','w',encoding='utf-8') as out:
            json.dump(dict(complete=complete,samples=samples),out,indent=2)
    if complete:
        ue.unregister_slate_post_tick_callback(soak_handle)

soak_handle = ue.register_slate_post_tick_callback(sample_soak)
