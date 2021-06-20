from setuptools import find_packages, setup

setup(
    name="fakedev",
    version="1.0.0",
    packages=find_packages(),
    install_requires=[
        "fastapi~=0.63.0",
        "typer~=0.3.2",
        "uvicorn~=0.13.4"
    ],
    entry_points={
        "console_scripts": ["fakedev=fakedev.main:main"]
    }
)
