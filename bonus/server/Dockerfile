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

EXPOSE 4242
EXPOSE 4243

CMD ["sh", "-c", "./zappy_server -p $PORT -x $WIDTH -y $HEIGHT -n $TEAMS -c $CLIENTS -f $FREQ --wait-timeout 2000"]
