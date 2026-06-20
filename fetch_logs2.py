import urllib.request
import json
import re

with open("builder_config.json", "r") as f:
    config = json.load(f)
    token = config.get("github_token")

class NoAuthRedirectHandler(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        m = req.get_method()
        if (code in (301, 302, 303, 307) and m in ("GET", "HEAD")
            or code in (301, 302, 303) and m == "POST"):
            newurl = newurl.replace(' ', '%20')
            newreq = urllib.request.Request(newurl,
                                            headers=req.headers,
                                            origin_req_host=req.origin_req_host,
                                            unverifiable=True)
            if 'Authorization' in newreq.headers:
                del newreq.headers['Authorization']
            return newreq
        else:
            raise urllib.error.HTTPError(req.full_url, code, msg, headers, fp)

opener = urllib.request.build_opener(NoAuthRedirectHandler)
urllib.request.install_opener(opener)

repo_path = "qwzxcv12/esp_pri"
run_id = "27860679016"

headers = {
    "Accept": "application/vnd.github.v3+json",
    "Authorization": f"token {token}",
    "User-Agent": "Python"
}

try:
    url = f"https://api.github.com/repos/{repo_path}/actions/runs/{run_id}/jobs"
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req) as response:
        jobs = json.loads(response.read().decode())['jobs']
        for job in jobs:
            if job['conclusion'] == 'failure':
                log_url = f"https://api.github.com/repos/{repo_path}/actions/jobs/{job['id']}/logs"
                log_req = urllib.request.Request(log_url, headers=headers)
                try:
                    with urllib.request.urlopen(log_req) as log_res:
                        log_text = log_res.read().decode()
                        print("\n--- ERROR LOGS ---")
                        lines = log_text.splitlines()
                        for line in lines[-200:]:
                            if "error" in line.lower() or "ninja" in line.lower() or "cpp" in line.lower():
                                print(line)
                except Exception as e:
                    print("Could not fetch log:", e)
except Exception as e:
    print("Error:", e)
