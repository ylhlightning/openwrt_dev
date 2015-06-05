#!/bin/sh 

log() {
	logger -t "TorrentDone" "$@"
}

TORRENT_DIR=$TR_TORRENT_DIR
TORRENT_NAME=$TR_TORRENT_NAME

DRV=$(uci get transmission.transmission.cfolder)

if [ -z $DRV ]; then
	exit 0
fi

if [ ! -e $DRV ]; then
	log "No Drive"
	QUE=$(uci get transmission.transmission.config_dir)
	if [ -z $QUE ]; then
		exit 0
	fi
	echo "$TORRENT_DIR" >> "$QUE/waiting"
	echo "$TORRENT_NAME" >> "$QUE/waiting"
	exit 0
fi

log "Copy Finished Torrent"
cp -R "$TORRENT_DIR/$TORRENT_NAME" "$DRV""/""$TORRENT_NAME"
