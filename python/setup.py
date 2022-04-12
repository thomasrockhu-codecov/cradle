from setuptools import setup

setup(
    name='cradle',
    version='0.0.1',
    description='CRADLE integration tests',
    license='MIT License',
    packages=['cradle'],
    package_dir={'': 'src'},
    install_requires=[
        'msgpack',
        'pytest',
        'websocket-client',
    ],
)
