from yaml import dump
from urllib.parse import urlparse
import re
from selenium import webdriver
from selenium.webdriver.firefox.options import Options
from sites import elitesparkle
from sites import psionicstorm
from sites import icyveins

options = Options()
options.headless = True
driver = webdriver.Firefox(options=options)

def parse_build_url(url):
    params = urlparse(url)
    obj = {}

    if (params.netloc == "www.icy-veins.com"):
        name = params.path
        name = name.replace("/heroes/", "")
        name = name.replace("-build-guide", "")
        obj["name"] = name.replace("-", "")
        obj["url"] = url
        obj["talents"] = icyveins.get_talents(driver, url)
    elif (params.netloc == "psionic-storm.com"):
        name = params.path
        name = name.replace("/#buildn=1", "")
        name = name.replace("/en", "")
        name = name.replace("/builds", "")
        name = name.replace("/", "")
        name = re.sub(r"-(\d){5}", "", name)
        obj["name"] = name.replace("-", "")
        obj["url"] = url
        obj["talents"] = psionicstorm.get_talents(driver, url)

    return obj

if __name__ == '__main__':
    # 1. Get build
    build_URLs = elitesparkle.get_build_links(driver)
    # build_URLs = ["https://www.icy-veins.com/heroes/the-lost-vikings-build-guide", "https://psionic-storm.com/en/builds/the-butcher-16393/#buildn=1"]
    # 2. For each build, retrieve the hero name,
    #    the talent string, and the URL.
    #    Add them to a list
    builds = []

    for build_URL in build_URLs:
        obj = parse_build_url(build_URL)
        builds.append(obj)

    # stop the driver
    driver.quit()

    builds_yaml = dump(builds)

    file = open("heroes_builds.yml", "w")

    file.write(builds_yaml)

    file.close()