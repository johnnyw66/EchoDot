#sudo docker run -it -p 514:514/udp -p 601:601 --name syslog-ng balabit/syslog-ng:latest -edv
docker run -it -p 514:514/udp -p 601:601 balabit/syslog-ng:latest -edv
