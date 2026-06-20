import urllib.request
import json
import re

# extract token from cloud_builder.py
with open("builder_config.json", "r") as f:
    config = json.load(f)
    token = config.get("github_token")


if not token:
    print("Could not find token")
    exit(1)

repo_path = "qwzxcv12/esp_pri"
run_id = "27860487253" # the last run

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
            print(f"Job: {job['name']}, Status: {job['conclusion']}")
            if job['conclusion'] == 'failure':
                log_url = f"https://api.github.com/repos/{repo_path}/actions/jobs/{job['id']}/logs"
                log_req = urllib.request.Request(log_url, headers=headers)
                try:
                    with urllib.request.urlopen(log_req) as log_res:
                        log_text = log_res.read().decode()
                        print("\n--- ERROR LOGS ---")
                        lines = log_text.splitlines()
                        for line in lines[-150:]:
                            if "error" in line.lower() or "warning" in line.lower() or "ninja" in line.lower() or "failed" in line.lower() or "cpp" in line.lower():
                                print(line)
                except Exception as e:
                    print("Could not fetch log:", e)
except Exception as e:
    print("Error:", e)
