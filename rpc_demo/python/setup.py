from setuptools import setup, find_packages

NAME = "app rpc"
DESC = "RPC api for the project."
VERSION = "0.1.0"

required = [
    "click",
    "rich",
    "PyYAML",
]

setup(
    name=NAME,
    version=VERSION,
    description=DESC,
    author='cdw',
    entry_points={
        #'console_scripts': [
        #    'config_gen=config_generator.config_generator:main'
        #],
    },
    packages=find_packages(),
    install_requires=required
)
