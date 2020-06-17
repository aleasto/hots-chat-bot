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
    driver.maximize_window()

    output = list("0000000")

    try:
        # Wait for the page to load and mark the correct talents
        talent_container = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.CSS_SELECTOR, ".talent-tree.live.active"))
        )
        # For each tier, for each talent, find the corrent one
        tiers = talent_container.find_elements_by_css_selector(".talent-tier")
        for tier_index, tier in enumerate(tiers):
            for talent_index, talent in enumerate(tier.find_elements_by_css_selector("li")):
                talent_classlist = talent.get_attribute("class")
                if ("talent-icon__container active" == talent_classlist):
                    # Replace the talent number in the talent string
                    output[tier_index] = str(talent_index + 1)
    except Exception as e:
        print("Error parsing " + url + " !")
        print(str(e))
    finally:
        # Very slow on my machine, perhaps driver.close() is faster, with a shared instance?
        driver.quit()
        # Print output
        talent_string = "T" + "".join(output)

        print("Done! (" + str(time() - time_start) +"s)")
        return talent_string
