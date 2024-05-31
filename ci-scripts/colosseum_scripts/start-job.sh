#!/bin/bash
set -xeu

AWX_API_URL=https://open6g-awx.colosseum.net
AWX_JOB_ID=15
AWX_JOB_EVENT_QUERY=job_events/?search=results_url
AWX_API_LAUNCH_PATH=/api/v2/job_templates/${AWX_JOB_ID}/launch/
AWX_MAX_API_CHECKS=120

COL_USER=$1
COL_PASS=$2
COL_VPN_URL=vpn.colosseum.net/open6g
PID_FILE=./openconnect_pid

# open vpn connection
set +x
sudo openconnect ${COL_VPN_URL} --useragent=AnyConnect --user=${COL_USER} --quiet --background --pid-file=${PID_FILE} --passwd-on-stdin < <(echo ${COL_PASS})
set -x

sleep 10

# launch job
set +x
curl -s -f -k -u ${COL_USER}:${COL_PASS} -X POST -H "Content-Type: application/json" -d '{"extra_vars": "{\"force_build\": \"true\", \"oai_branch\": \"develop\", \"colosseum_rf_scenario\": \"10011\"}"}' https://open6g-awx.colosseum.net/api/v2/job_templates/16/launch/ > launch.json
set -x

AWX_API_JOB_PATH=$(jq -r '.url' launch.json)

# wait for job to complete
for try in $(seq ${AWX_MAX_API_CHECKS}); do
  set +x
  curl -s -f -k -u ${COL_USER}:${COL_PASS} -X GET ${AWX_API_URL}${AWX_API_JOB_PATH} > status.json
  set -x

  FINISHED=$(jq -r '.finished' status.json)
  [ "${FINISHED}" = "null" ] || break

  sleep 60
done

[ $try != $AWX_MAX_API_CHECKS ] || echo "WARNING: stopped retrying after $AWX_MAX_API_CHECKS times; timed out?"

echo "AWX job completed $FINISHED"

# get result url and download test results
set +x
curl -s -f -k -u ${COL_USER}:${COL_PASS} -X GET ${AWX_API_URL}${AWX_API_JOB_PATH}${AWX_JOB_EVENT_QUERY} > result.json
set -x

RESULT=$(jq -r '.results[0].event_data.res.ansible_facts.results_url' result.json)
wget -q ${RESULT}

# close vpn connection
sudo kill -2 $(cat ${PID_FILE})

STATUS=$(jq -r '.status' status.json)
echo "job status: ${STATUS}"
[ "${STATUS}" = "successful" ]  # sets final exit code and thus jenkins pass or fail
