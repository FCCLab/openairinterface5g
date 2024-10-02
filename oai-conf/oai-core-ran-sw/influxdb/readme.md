```
sudo chown -R tuannv:tuannv *
tar -czvf  influxdb.tar.gz data config
```

```
tar -xzvf  influxdb.tar.gz
```

```
username: admin
password: admin123
```

```
git lfs migrate import --include="*.tar.gz"
```

```
cd ..
docker compose -f  docker-compose-db.yaml up
```
