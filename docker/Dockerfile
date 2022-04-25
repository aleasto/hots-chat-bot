FROM ubuntu:21.10

RUN apt-get update -y && apt-get install -y wget procps psmisc xvfb x11vnc fluxbox

RUN dpkg --add-architecture i386 \
    && wget -nc https://dl.winehq.org/wine-builds/winehq.key -O /usr/share/keyrings/winehq-archive.key \
    && wget -nc https://dl.winehq.org/wine-builds/ubuntu/dists/impish/winehq-impish.sources -P /etc/apt/sources.list.d/ \
    && apt-get update -y && apt-get install -y winehq-staging

RUN apt-get update -y && apt-get install -y libgnutls30:i386 libgpg-error0:i386 libsqlite3-0:i386 winbind

RUN wget "https://www.battle.net/download/getInstallerForGame?os=win&gameProgram=HEROES_OF_THE_STORM&version=Live&id=undefined" -O hots-installer.exe

ENV DISPLAY :99
COPY start-xvfb.sh .
RUN chmod +x start-xvfb.sh

RUN mkdir ~/.fluxbox && cp ./etc/X11/fluxbox/keys ~/.fluxbox/keys && echo $'\n\
OnWindow Mod4 Mouse1 :MacroCmd {Raise} {Focus} {StartMoving}\n\
OnWindow Mod4 Mouse3 :MacroCmd {Raise} {Focus} {StartResizing NearestCorner}\n\
' >> ~/.fluxbox/keys

RUN ./start-xvfb.sh wine ./hots-installer.exe
RUN cp -r /root/.wine/drive_c/Program\ Files\ \(x86\)/Heroes\ of\ the\ Storm/Support64/* /root/.wine/drive_c/windows/system32/

EXPOSE 5900
ENV VNCPASSWD=
ENV WINEDLLOVERRIDES="nvapi,nvapi64="
CMD ./start-xvfb.sh wine /root/.wine/drive_c/Program\ Files\ \(x86\)/Battle.net/Battle.net.exe
