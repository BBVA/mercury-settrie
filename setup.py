import os, warnings, setuptools

from setuptools.command.build_py import build_py as _build_py
from src.version import __version__


def warn(*args, **kwargs):
	pass

warnings.warn = warn


settrie_ext = setuptools.Extension(name				= 'src.settrie._py_settrie',
								 sources			= ['src/settrie/settrie.cpp', 'src/settrie/py_settrie_wrap.cpp'],
								 extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])


class custom_build_py(_build_py):

	def run(self):
		self.run_command('build_ext')

		dis = self.distribution if type(self.distribution) != list else self.distribution[0]
		cob = dis.command_obj['build_ext']

		fn = cob.build_lib
		assert 'src' in os.listdir(fn)

		fn += '/src'
		assert 'settrie' in os.listdir(fn)

		fn += '/settrie'

		fnl = os.listdir(fn)
		assert len(fnl) == 1

		fn += '/' + fnl[0]

		ret = super().run()

		self.move_file(fn, fn.replace('src/settrie', 'settrie'))

		return ret


setuptools.setup(
    name			 = 'mercury-settrie',
    version			 = __version__,
    description		 = 'A SetTrie is a container of sets that performs efficient subset and superset queries.',
	long_description = """    Settrie was born from the need of a better implementation of the algorithm for our recommender system. It has
		direct application to text indexing for search in logarithmic time of documents containing a set of words. Think of a collection of
		documents as a container of the set of words that each document has. Finding all the documents that contain a set of words is
		finding the superset of the set of words in the query. A SetTrie will give you the answer --the list of document names-- in
		logarithmic time. The data structure is also used in auto-complete applications.""",
	url				 = 'https://github.com/BBVA/mercury-settrie',
	license			 = 'Apache 2',
	platforms		 = ['Linux', 'MacOS', 'Windows'],
	classifiers		 = ['Programming Language :: Python :: 3',
		'License :: OSI Approved :: Apache Software License',
		'Operating System :: OS Independent'],
	keywords		 = ['settrie', 'recommender systems', 'auto complete'],
    packages		 = ['settrie'],
    package_dir		 = {'' : 'src'},
	python_requires	 = '>=3.8',
	cmdclass		 = {'build_py' : custom_build_py},
    ext_modules		 = [settrie_ext]
)
