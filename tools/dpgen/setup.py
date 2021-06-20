from setuptools import setup

setup(
    name="dpgen",
    version="1.0.0",
    packages=["dpgen"],
    install_requires=[
        "PyYAML>=5.4.1,<5.5.0"
    ],
    entry_points={
        "console_scripts": ["dpgen=dpgen:main"]
    }
)
