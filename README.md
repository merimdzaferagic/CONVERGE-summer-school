# CONVERGE-summer-school
## 1. Install Docker and Docker-compose
Run the following script to install docker and docker-compose:

```shell
#!/bin/bash
apt-get remove docker docker-engine docker.io containerd runc
apt-get update
apt-get install -y ca-certificates curl gnupg lsb-release
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo \
    "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu \
      $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null

apt-get update
apt-get install -y python3-pip
apt-get install -y docker-ce docker-ce-cli containerd.io
apt-cache madison docker-ce

systemctl enable docker.service
systemctl enable containerd.service


#install docker-compose
mkdir -p ~/.docker/cli-plugins/
curl -SL https://github.com/docker/compose/releases/download/v2.2.3/docker-compose-linux-x86_64 -o ~/.docker/cli-plugins/docker-compose
echo "step 3"
chmod +x ~/.docker/cli-plugins/docker-compose
chown $USER /var/run/docker.sock


cp ~/.docker/cli-plugins/docker-compose /usr/local/bin/docker-compose

docker-compose version


systemctl enable docker.service
systemctl enable containerd.service
```

## 2. srsRAN setup
This is srsRAN_Project commit: 644263b5a7c7b98b892bc1120940ae8d801eaee0





## To run the xapp with ORAN-SC:
```shell
sudo docker compose exec python_xapp_runner ./kpm_mon_xapp.py --metrics=CQI,RSRP,RRU.PrbAvailDl,RRU.PrbAvailUl,RRU.PrbUsedDl,RRU.PrbUsedUl,RRU.PrbTotDl,RRU.PrbTotUl,DRB.AirIfDelayUl,RACH.PreambleDedCell --kpm_report_style=1
```

List of metrics: https://github.com/srsran/srsRAN_Project/blob/main/lib/e2/e2sm/e2sm_kpm/e2sm_kpm_du_meas_provider_impl.cpp
