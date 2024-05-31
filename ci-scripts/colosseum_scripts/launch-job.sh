#!/bin/bash
set -eu

AWX_API_URL=https://open6g-awx.colosseum.net
AWX_JOB_ID=16
AWX_API_LAUNCH_PATH=/api/v2/job_templates/${AWX_JOB_ID}/launch/

COL_USER=$1
COL_PASS=$2
JENKINS_JOB_ID=$3
GIT_REPOSITORY=$4
GIT_BRANCH=$5

# launch job
curl -s -f -k -u ${COL_USER}:${COL_PASS} -X POST -H "Content-Type: application/json" -d '{"extra_vars": "{\"oai_repo\": \"'${GIT_REPOSITORY}'\", \"oai_branch\": \"'${GIT_BRANCH}'\", \"colosseum_rf_scenario\": \"10011\", \"jenkins_job_id\": \"'${JENKINS_JOB_ID}'\"}"}' ${AWX_API_URL}${AWX_API_LAUNCH_PATH} > launch.json
