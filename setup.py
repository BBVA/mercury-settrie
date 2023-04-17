from setuptools import setup

from _custom_build import settrie_ext


setup(
    name		= 'settrie',
    version		= '1.4.1',
    description	= 'A SetTrie is a container of sets that performs efficient subset and superset queries.',
    packages	= ['mercury-settrie'],
    ext_modules	= [settrie_ext]
)
