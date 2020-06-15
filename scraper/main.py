from requests import get
from requests.exceptions import RequestException
from contextlib import closing
from bs4 import BeautifulSoup

from sites import psionicstorm

if __name__ == '__main__':
    psionicstorm.get_talents(
        "https://psionic-storm.com/en/builds/anubarak-16405/#buildn=1")
