# gtk-xrandr-gamma
Small graphical tool for previewing xrandr settings live

![screenshot](https://i.imgur.com/Hq97f6S.png)

## Dependencies
- GTK 3
- pkg-config

debian:
```bash
sudo apt install libgtk-3-dev pkg-config
```

## Running

```bash
git clone "https://github.com/rydz/gtk-xrandr-gamma.git"
cd gtk-xrandr-gamma
make run
```

## TODO
- Store slider settings relative to each display
- Redshift integration
