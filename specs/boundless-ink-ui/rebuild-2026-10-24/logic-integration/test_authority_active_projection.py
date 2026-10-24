"""Behavioral authority projection and native-rules regression in isolated test DBs."""
from pathlib import Path
import sys,unittest
ROOT=Path(__file__).resolve().parents[4]
sys.path[:0]=[str(ROOT),str(ROOT/'Tools/CultivationService/tests')]
from test_foundation import ServiceCase

class ActiveProjectionTests(ServiceCase):
    def assert_real_flag(self,token,expected):
        state=self.service.snapshot(token)
        self.assertIn('active',state['identity'])
        self.assertIs(type(state['identity']['active']),bool)
        with self.service.store.transaction() as db:
            actual=bool(db.execute('SELECT active FROM session WHERE token=?',(token,)).fetchone()[0])
        self.assertIs(state['identity']['active'],actual)
        self.assertIs(state['identity']['active'],expected)
    def test_new_session_exports_actual_inactive(self):self.assert_real_flag(self.anchor,False)
    def test_sync_true_exports_actual_active(self):
        reply=self.service.sync(self.anchor,{'active':True})
        self.assertIs(reply['identity'].get('active'),True)
        self.assert_real_flag(self.anchor,True)
    def test_sync_false_revokes_projected_flag(self):
        self.service.sync(self.anchor,{'active':True});reply=self.service.sync(self.anchor,{'active':False})
        self.assertIs(reply['identity'].get('active'),False)
        self.assert_real_flag(self.anchor,False)
    def test_viewer_participation_uses_its_real_session_flag(self):
        viewer=self.session('projection-viewer','viewer')
        self.service.sync(self.anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        self.assert_real_flag(viewer,True)
        self.service.sync(self.anchor,{'active':False,'participants':[{'token':viewer,'active':True}]})
        self.assert_real_flag(viewer,False)
    def team(self,user,room):
        a=self.session(user,'anchor',room);g=self.command(a,'CreateGuild',{'name':'投影'+user})['result']['guild']['id'];v=self.session(user+'-viewer','viewer',room)
        self.assertEqual(self.command(v,'JoinGuild',{'guild_id':g})['status'],'success')
        self.service.sync(a,{'active':True,'participants':[{'token':v,'active':True}]});return a,v
    def running(self):
        teams=[self.team('甲','a'),self.team('乙','b')]
        for a,v in teams:self.assertEqual(self.command(a,'QueueRace',{'node_id':'NODE_STONE'})['status'],'success')
        start=self.service.snapshot(teams[0][0])['race']['start'];self.now=start+119
        for a,v in teams:self.service.sync(a,{'active':True,'participants':[{'token':v,'active':True}]})
        self.now=start+121;return teams,self.service.snapshot(teams[0][0])['race']
    def test_active_eligible_native_race_action_still_succeeds(self):
        teams,race=self.running();a,v=teams[0];self.assert_real_flag(a,True)
        result=self.command(a,'SubmitRaceAction',{'race_id':race['id'],'kind':'common','action':'护阵'})
        self.assertEqual(result['status'],'success',result)
    def test_inactive_native_race_action_still_refused(self):
        teams,race=self.running();a,v=teams[0];self.service.sync(a,{'active':False});self.assert_real_flag(a,False)
        result=self.command(a,'SubmitRaceAction',{'race_id':race['id'],'kind':'common','action':'护阵'})
        self.assertEqual(result['error_code'],'PARTICIPATION_REQUIRED',result)
    def test_stale_heartbeat_rule_is_not_replaced_by_exported_flag(self):
        teams,race=self.running();a,v=teams[0];self.now+=6;self.assert_real_flag(a,True)
        result=self.command(a,'SubmitRaceAction',{'race_id':race['id'],'kind':'common','action':'护阵'})
        self.assertEqual(result['error_code'],'PARTICIPATION_REQUIRED',result)

if __name__=='__main__':unittest.main(verbosity=2)
