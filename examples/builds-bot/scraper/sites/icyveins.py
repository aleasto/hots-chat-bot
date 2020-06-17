from selenium import webdriver
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from time import time

def get_talents(driver, url):
    """
    Attempts to get the talents of a build at
    the specified page. The result is a talent string ("T0000000").
    If there's something wrong,
    it will return a partially invalid talent string.

    """
    print("Parsing " + url + " ...")
    time_start = time()

    driver.get(url)

    output = "T0000000"

    try:
        # Wait for the page to load and mark the correct talents
        talent_button = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.CSS_SELECTOR, ".talent_build_copy_button"))
        )
        build_button = talent_button.find_element_by_css_selector("input[type=\"hidden\"]")
        build = build_button.get_attribute("value")
        build = build.replace("[", "")
        build = build.replace("]", "")
        tokens = build.split(",")
        output = tokens[0]
    except Exception as e:
        print("Error parsing " + url + " !")
        print(str(e))
    finally:
        # Very slow on my machine, perhaps driver.close() is faster, with a shared instance?
        # driver.quit()
        # Print output
        print("Done! (" + str(time() - time_start) +"s)")
        return output
        
