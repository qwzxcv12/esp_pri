import urllib.request, json
with open("linux_builder_config.json") as f:
    config = json.load(f)
token = config["github_token"]
req = urllib.request.Request("https://api.github.com/repos/qwzxcv12/esp_pri/actions/runs/29625855650/jobs")
req.add_header("Authorization", f"Bearer {token}")
req.add_header("Accept", "application/vnd.github.v3+json")
res = urllib.request.urlopen(req)
jobs = json.loads(res.read())
job_id = jobs["jobs"][0]["id"]
log_url = f"https://api.github.com/repos/qwzxcv12/esp_pri/actions/jobs/{job_id}/logs"
req = urllib.request.Request(log_url)
req.add_header("Authorization", f"Bearer {token}")
try:
    log_res = urllib.request.urlopen(req)
    with open("build_error.log", "wb") as logf:
        logf.write(log_res.read())
except Exception as e:
    print(e)
