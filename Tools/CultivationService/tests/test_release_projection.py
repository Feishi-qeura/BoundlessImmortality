from test_foundation import ServiceCase


class ReleaseProjectionTests(ServiceCase):
    def test_records_carry_the_authority_realm_caption_for_rights_page(self):
        viewer = self.session('viewer-caption', 'viewer')
        anchor_state = self.service.snapshot(self.anchor)
        viewer_state = self.service.snapshot(viewer)
        for token, state in ((self.anchor, anchor_state), (viewer, viewer_state)):
            record = next(r for r in state['records'] if r['user'] == state['identity']['user'])
            self.assertEqual(record.get('realm_name'), state['growth']['name'])
            self.assertTrue(record['realm_name'])
