import os, shutil


# Move notebooks inside src/settrie before packaging
if os.path.exists('notebooks'):
	shutil.move('notebooks', 'src/settrie/tutorials')


from setuptools import setup, find_packages, Extension


settrie_ext = Extension(name				= 'settrie._py_settrie',
						sources				= ['src/settrie/settrie.cpp', 'src/settrie/py_settrie_wrap.cpp'],
						include_dirs		= ['src/settrie'],
						extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])

setup_args = dict(
	packages			 = find_packages(where = 'src'),
	package_dir			 = {'' : 'src'},
	ext_modules			 = [settrie_ext],
	scripts				 = ['src/test_all.py'],
	include_package_data = True,
	package_data		 = {'mypackage': ['tutorials/*', 'tutorials/data/*']}
)

setup(**setup_args)
