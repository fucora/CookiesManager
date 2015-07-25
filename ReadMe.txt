Manage cookies in Mozilla Firefox with functions such as add, clear, auto clear, edit and autorestore after startup.

Create cookies.cfg in firefox profile folder (default stores in %APPDATA%\Mozilla\Firefox\Profiles).
CookiesManager uses only to create configuration file. It file contains groupname (begins with #) and rows. Rows contains Host, IsHttpOnly flag, path, IsSecure flag, expire date, name and value. The first group is obligatory (may be empty) - it favorite group.

Add settings to Firefox in about:config:
usersettings.cm.apppath (string) - Full path to executable file of CookiesManager (e.g. "C:\App\CookiesManager.exe");
usersettings.cm.timer.enabled (boolean) - Auto clear cookies is enabled (default false);
usersettings.cm.timer.interval (integer) - Auto clear interval in milliseconds (default 300000).

Install CustomButtons addon (https://addons.mozilla.org/ru/firefox/addon/custom-buttons)
Add new button (copy-paste Button\Button.txt in addressbar), move icon to bar in customize settings.

Icon actions:
Left-click - Show all cookies in tab;
Middle-click - Clear cookies (without cookies in favorite group);
Right-click - Choose group for add cookies (remove all previous cookies for the host, said in a cookie);
Ctrl-click - Toggle auto clear cookies (without cookies in favorite group and currently open sites in tabs);
Shift-click - Open CookiesManager for current profile.

After startup all cookies is removed without favorite group.
