def test_calculation_request(session):
    calc_id = '61e53771010054e512d49dce6f3ebd8b'

    expected = \
        { 'calculation': { 'function': { 'account': 'mgh',
                                         'app': 'dosimetry',
                                         'args': ( { 'reference': '61e5377001001d0206ce037e602e97a5'},
                                                   { 'value': 0.02764318603944116}),
                                         'name': 'addition'}}
          }

    cmds = []
    for i in range(3):
        cmds.append(session.start_calculation_request(calc_id))
    for cmd in cmds:
        new_result = session.receive_response(cmd)
        assert new_result == expected
