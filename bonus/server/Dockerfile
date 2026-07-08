FROM epitechcontent/epitest-docker

WORKDIR /app

COPY . .

RUN make re

ENV PORT=4242
ENV WIDTH=10
ENV HEIGHT=10
ENV TEAMS=team1
ENV CLIENTS=3
ENV FREQ=100
# Serial-turn mode timeout in ms; set WAIT_TIMEOUT="" to run in classic
# (non-blocking) mode — much faster for playbook scenes with many agents.
ENV WAIT_TIMEOUT=2000

EXPOSE 4242
EXPOSE 4243

CMD ["sh", "-c", "./zappy_server -p $PORT -x $WIDTH -y $HEIGHT -n $TEAMS -c $CLIENTS -f $FREQ ${WAIT_TIMEOUT:+--wait-timeout $WAIT_TIMEOUT}"]
