from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

from time import time, sleep

elitesparkle_URL = "https://elitesparkle.wixsite.com/hots-builds"

NUMBER_OF_HEROES = 88

def get_build_links(driver):
    """
    Attempts to get all the links to the builds
    from Elitesparkle's website.
    Returns a list of strings (URLs).
    """
    
    print("Start parsing elitesparkle's website...")
    time_start = time()

    driver.get(elitesparkle_URL)

    current_heroes = 0
    builds = []
    try:
        while (current_heroes < NUMBER_OF_HEROES):
            # wait a second
            sleep(1)
            # Wait for the page to load and mark the correct talents
            heroes_container = driver.find_element_by_css_selector("#c1dmpinlineContent")
            # For each tier, for each talent, find the corrent one
            heroes = heroes_container.find_elements_by_css_selector("a")
            current_heroes = len(heroes)
        # We now have all heroes
        for hero in heroes:
            build_link = hero.get_attribute("href")
            # print(build_link)
            builds.append(build_link)
    except Exception as e:
        print("Error parsing elitesparkle's website")
        print(str(e))
    finally:
        # Very slow on my machine, perhaps driver.close() is faster, with a shared instance?
        # driver.quit()
        print("Done! (" + str(time() - time_start) +"s)")
        return builds
