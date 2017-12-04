FROM buildpack-deps:jessie

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN apt-get update && apt-get install -y \
  scons \
  libsnappy-dev \
  libboost-dev

RUN cd /root && \
  git clone https://github.com/ifwe/bruce.git && \
  cd bruce/src/bruce && \
  scons -Q --up --release bruce

RUN mkdir -p /opt/bruce/bin/
RUN cp /root/bruce/out/release/bruce/bruce /opt/bruce/bin/

RUN mkdir -p /etc/bruce
# ADD bruce_conf.xml /etc/bruce/

EXPOSE 9090

CMD /opt/bruce/bin/bruce --daemon --log_echo --log_level LOG_DEBUG --msg_buffer_max 65536 --discard_log_path /root/discard_msgs.txt --receive_socket_name /root/socket/bruce.socket --config_path /etc/bruce/bruce_conf.xml
