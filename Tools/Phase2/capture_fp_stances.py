import unreal,time
fworld=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
fplayer=unreal.GameplayStatics.get_player_pawn(fworld,0)
fcontroller=unreal.GameplayStatics.get_player_controller(fworld,0)
fplayer.set_actor_location(unreal.Vector(-500,0,-1.8),False,True)
def fkey(k,on=True):unreal.Phase2Tools.test_key(fcontroller,k,on)
def fshot(n):unreal.SystemLibrary.execute_console_command(fworld,'Shot SHOWUI filename=C:/GameDev/AfghanistanFPS/Saved/Phase2/'+n+'.png -nosuffix')
fsteps=[(1.0,None,None),(0.6,lambda:fshot('fp_standing'),None),
        (1.0,None,lambda:fkey('LeftControl')),(0.6,lambda:fshot('fp_crouching'),lambda:fkey('LeftControl')),
        (1.0,lambda:(fkey('LeftControl',False),fkey('Z')),None),(0.6,lambda:(fkey('Z',False),fshot('fp_prone')),None)]
fstate={'i':0,'start':0,'entered':False}
def ftick(dt):
    if fstate['i']>=len(fsteps):
        unreal.unregister_slate_post_tick_callback(fstate['h']);print('P2_FP_STANCES_DONE');return
    dur,enter,during=fsteps[fstate['i']]
    if not fstate['entered']:
        fstate['entered']=True;fstate['start']=time.monotonic()
        if enter:enter()
    if during:during()
    if time.monotonic()-fstate['start']>=dur:fstate['i']+=1;fstate['entered']=False
fstate['h']=unreal.register_slate_post_tick_callback(ftick)
print('P2_FP_STANCES_STARTED')
