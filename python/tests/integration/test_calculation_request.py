def test_calculation_request(session):
    calc_id = '61e53771010054e512d49dce6f3ebd8b'

    assert session.calculation_request(calc_id) == \
        { 'calculation': { 'function': { 'account': 'mgh',
                                         'app': 'dosimetry',
                                         'args': ( { 'reference': '61e5377001001d0206ce037e602e97a5'},
                                                   {'value': 0.02764318603944116}),
                                         'name': 'addition'}}
          }
