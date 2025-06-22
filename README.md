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

## 3. Build basic setup
Change directory to `srsRAN/docker/` and build the basic setup by ensuring that the `docker-compose` file with no RICs is being used:
```shell
cp docker-compose_no_ric.yml docker-compose.yml
docker-compose build
```

Once the building process finishes, you can run the setup by simply running:
```shell
docker-compose up
```

## 4. Inspect basic setup
Inspect the configuration files:
1. `docker-compose.yml`
2. `srsRAN_conf/gnb_config_no_ric.yml`
3. `srsRAN_conf/testmode.yml`

Attach to the `srsran_gnb` container and type `t` to see console:
```shell
sudo docker attach srsran_gnb
```

Open the Grafana dashboard to inspect the setup. In your browser open: `http://localhost:3300/`

## 5. Simple file configuration modification
Try to modify the following parameters and inspect the impact:
- Number of users connected
- Bandwidth used
- Subcarrier spacing
- Max MCS

## 6. FlexRIC build and run
Prepare the correct docker-compose file:
```shell
cp docker-compose_flex_ric.yml docker-compose.yml
```

Build the missing containers (flexric and xappmon):
```shell
sudo docker-compose build flexric xappmon
```

Run all services
```shell
sudo docker-compose up
```

Inspect the configuration files for the ric and the xapp. These files can be found in the flexRIC folder (`ric.conf` and `xapp_mon_e2sm_kpm.conf`).


## 7. ORAN-SC RIC build and run
Prepare the correct docker-compose file:
```shell
cp docker-compose_oranSC.yml docker-compose.yml
```

Build the missing containers (dbaas rtmgr_sim submgr e2term appmgr e2mgr python_xapp_runner):
```shell
sudo docker-compose build dbaas rtmgr_sim submgr e2term appmgr e2mgr python_xapp_runner
```

Run all services:
```shell
sudo docker-compose up
```

To run the xapp with ORAN-SC:
```shell
sudo docker compose exec python_xapp_runner ./kpm_mon_xapp.py --metrics=CQI,RSRP,RRU.PrbAvailDl,RRU.PrbAvailUl,RRU.PrbUsedDl,RRU.PrbUsedUl,RRU.PrbTotDl,RRU.PrbTotUl,DRB.AirIfDelayUl,RACH.PreambleDedCell --kpm_report_style=1
```

List of metrics: https://github.com/merimdzaferagic/CONVERGE-summer-school/blob/main/srsRAN/lib/e2/e2sm/e2sm_kpm/e2sm_kpm_du_meas_provider_impl.cpp

Inspect the configuration files for the ric and the xapp. These files can be found in the `oran-sc-ric` folder (`oran-sc-ric/xApps/python/kpm_mon_xapp.py`).
