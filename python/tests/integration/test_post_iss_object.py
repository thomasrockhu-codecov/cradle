def test_post_iss_object(session):
    objA = ('abc', 'def', 'ghi')
    schema = 'array/string'

    objA_id = session.post_iss_object(schema, objA)
    assert session.get_iss_object(objA_id) == objA
