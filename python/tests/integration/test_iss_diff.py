def test_iss_diff(session):
    objA = ('abc', 'def', 'ghi')
    objB = ('abc', 'xyz', 'ghi')
    schema = 'array/string'

    objA_id = session.post_iss_object(schema, objA)
    assert session.get_iss_object(objA_id) == objA
    objB_id = session.post_iss_object(schema, objB)
    assert session.get_iss_object(objB_id) == objB

    assert session.iss_diff(objA_id, objB_id) == \
        (
            {
                'diff': ({'a': {'some': 'def'}, 'b': {'some': 'xyz'}, 'op': 'update', 'path': (1,)},),
                'id_in_a': objA_id,
                'id_in_b': objB_id,
                'path_from_root': (),
                'service': 'iss'
            },)
