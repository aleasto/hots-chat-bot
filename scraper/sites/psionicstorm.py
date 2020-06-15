from requests import get
from requests.exceptions import RequestException
from bs4 import BeautifulSoup

def get_talents(url):
    """
    Attempts to get the talents of a build at
    the specified page. If there's something wrong,
    it will return null.
    """
    try:
        with get(url, stream=True) as res:
            if is_res_valid(res):
                return parse_page(res)
            else:
                return None
    except RequestException as e:
        handle_error(str(e))
        return None

def is_res_valid(res):
    """
    Returns True if the response is HTML, False otherwise.
    """
    content_type = res.headers['Content-Type'].lower()
    return (res.status_code == 200
            and content_type is not None
            and content_type.find('html') > -1)

def handle_error(e):
    print(e)

def parse_page(res):
    raw_html = res.content
    
    html = BeautifulSoup(raw_html, 'html.parser')

    t_tiers = html.select(".talent-tree.live.active > .talent-tier")

    for tier in t_tiers:
        tier_talents = tier.select("li")
        for index, talent in enumerate(tier_talents):
            if (talent.has_attr("class")):
                print(talent["class"])
    return 0

