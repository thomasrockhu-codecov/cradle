def test_calculation_diff(session):
    calcA_id = '61e53771010054e512d49dce6f3ebd8b'
    calcB_id = '61e7dee9010099f9d50b5cefc7e418e8'

    assert session.calculation_diff(calcA_id, calcB_id) == \
        ( { 'diff': ( { 'a': {'some': 0.4851082858232464},
                        'b': {'some': 0.8860139318754435},
                        'op': 'update',
                        'path': ('function', 'args', 0, 'value')},
                      { 'a': {'some': 0.21176340864751386},
                        'b': {'some': 0.6999000517821027},
                        'op': 'update',
                        'path': ('function', 'args', 1, 'value')}),
            'id_in_a': '61e5377001001d0206ce037e602e97a5',
            'id_in_b': '61e7dee90100f0cf996ef551da01bf83',
            'path_from_root': ('function', 'args', 0),
            'service': 'calc'},
          { 'diff': ( { 'a': {'some': 0.02764318603944116},
                        'b': {'some': 0.3254067151882709},
                        'op': 'update',
                        'path': ('function', 'args', 1, 'value')},),
            'id_in_a': '61e53771010054e512d49dce6f3ebd8b',
            'id_in_b': '61e7dee9010099f9d50b5cefc7e418e8',
            'path_from_root': (),
            'service': 'calc'}
          )
