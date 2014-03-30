'''
@author: raptor

One script to rule them all...  
   
-------
USAGE
-------

If bitfighter is installed on a custom path, include it as an argument.
e.g.
bitfighter_notifier.py C:\my\custom\path\bitfighter.exe
   
   
Platform-specific notes:

-------
WINDOWS
-------

You need the following packages to run this:
 - pywin32 - http://sourceforge.net/projects/pywin32/files/pywin32/
   Note that the it is important to get the correct version -- see README.txt in the pywin32 downloads section for details
 - systray - http://sourceforge.net/projects/pysystray/files/pysystray/

 This is known to work with Python 2.7


------- 
OSX
-------

Works on OSX 10.6+, but better on 10.8+ where we can take advantage of the
built-in notification system.

It may work on OSX 10.5 or 10.4 if PyOBJC is installed


-------
LINUX
-------

The Linux part will attempt to use one of the following as the GUI:
 - QT
 - GTK2 through PyGTK
 - GTK3 (also maybe GTK2) through PyGObject
 
It will also attempt to use one of the following for the messaging system:
 - Notify2
 - DBus
 - notify-send command

The precedence order of which to check first can be changed


'''


"""
Common Imports
"""
import functools
import inspect
import json
import logging
import os
import subprocess
import sys
import threading
import tempfile   # For our singleton class

  
"""
Configuration
"""
APPLICATION_NAME        = "Bitfighter Notifier Applet"
GUI_TITLE               = "Bitfighter"
MESSAGE_TITLE           = "Bitfighter"
URL                     = "http://bitfighter.org/bitfighterStatus.json"
REFRESH_INTERVAL        = 20
NOTIFICATION_TIMEOUT    = 5
EXECUTABLE              = "bitfighter"

# Different icon formats for different systems
if sys.platform == 'win32':
    ICON_PATH           = "redship48.ico"

elif sys.platform == 'darwin':
    ICON_PATH           = "redship18.png"

else:
    ICON_PATH           = "redship48.png"


loggingLevel = logging.ERROR


'''
Developer specific overrides
'''
# For testing and debugging...
devMode = False

if devMode == True:
    REFRESH_INTERVAL = 5
    loggingLevel = logging.DEBUG


"""
Logging
"""
logging.basicConfig(level=loggingLevel)


"""
Singleton class

This prevents multiple notifiers from running.  Taken from:
   https://github.com/pycontribs/tendo/blob/master/tendo/singleton.py
"""
class SingleInstance:
    def __init__(self, flavor_id=""):
        self.initialized = False
        basename = os.path.splitext(os.path.abspath(sys.argv[0]))[0].replace("/", "-").replace(":", "").replace("\\", "-") + '-%s' % flavor_id + '.lock'
        # os.path.splitext(os.path.abspath(sys.modules['__main__'].__file__))[0].replace("/", "-").replace(":", "").replace("\\", "-") + '-%s' % flavor_id + '.lock'
        self.lockfile = os.path.normpath(tempfile.gettempdir() + '/' + basename)

        logging.debug("SingleInstance lockfile: " + self.lockfile)
        if sys.platform == 'win32':
            try:
                # file already exists, we try to remove (in case previous execution was interrupted)
                if os.path.exists(self.lockfile):
                    os.unlink(self.lockfile)
                self.fd = os.open(self.lockfile, os.O_CREAT | os.O_EXCL | os.O_RDWR)
            except OSError:
                type, e, tb = sys.exc_info()
                if e.errno == 13:
                    logging.error("Another instance is already running, quitting.")
                    sys.exit(-1)
                print(e.errno)
                raise
        else:  # non Windows
            import fcntl
            self.fp = open(self.lockfile, 'w')
            try:
                fcntl.lockf(self.fp, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except IOError:
                logging.warning("Another instance is already running, quitting.")
                sys.exit(-1)
        self.initialized = True

    def __del__(self):
        import sys
        import os
        if not self.initialized:
            return
        try:
            if sys.platform == 'win32':
                if hasattr(self, 'fd'):
                    os.close(self.fd)
                    os.unlink(self.lockfile)
            else:
                import fcntl
                fcntl.lockf(self.fp, fcntl.LOCK_UN)
                # os.close(self.fp)
                if os.path.isfile(self.lockfile):
                    os.unlink(self.lockfile)
        except Exception as e:
            if logging:
                logging.warning(e)
            else:
                print("Unloggable error: %s" % e)
            sys.exit(-1)

# Use our singleton
me = SingleInstance()


"""
Utility classes
"""
# Adapted from a random pastebin on the web: http://pastebin.com/1VcumfC3
class PeriodicTimer(object):
    def __init__(self, interval, callback):
        self.interval = interval
 
        @functools.wraps(callback)
        def wrapper(*args, **kwargs):
            # Do the action!
            result = callback(*args, **kwargs)

            if result:
                # Restart a Timer to call this same action again
                self.thread = threading.Timer(self.interval, self.callback)
                self.thread.start()
 
        self.callback = wrapper
 
    def start(self):
        self.thread = threading.Timer(self.interval, self.callback)
        self.thread.start()
 
    def cancel(self):
        self.thread.cancel()


'''
Base Classes

These classes are shared across all platforms
'''
        
"""
PlayerListReceiver
 
Downloads and parses the online JSON
"""
class PlayersListReceiver(object):
    def __init__(self, url, messenger, guiApp):
        self.players = set()
        self.url = url
        self.messenger = messenger
        self.guiApp = guiApp
        self.firstRun = True
        self.hasConnectivity = True
        

    def fetch(self):
        # Python version 3 uses this
        if sys.version_info >= (3, 0):
            import urllib.request
            fp = urllib.request.urlopen(self.url)
            bytesInf = fp.read()
            strInf = bytesInf.decode("utf8")
            fp.close()
            return strInf
        # Python 2.x
        else:
            import urllib2
            response = urllib2.urlopen(self.url)
            return response.read()
        
            
    # This method is run in a separate thread and call        
    def refresh(self):
        logging.debug("Refreshing JSON")
        
        try:
            gameInf = json.loads(self.fetch(), strict=False)
            self.hasConnectivity = True
        except:
            logging.debug("Unable to fetch data from {0}".format(self.url))

            # We just lost internet connectivity            
            if self.hasConnectivity == True:
                # Empty player set
                self.players = set()
                self.guiApp.refreshToolTip(self.players)
            
            
            self.hasConnectivity = False

            # Continue anyways - we may have lost connectivity for only a short while
            return True
        
        
        playersNew = set(gameInf["players"])
        
        if playersNew != self.players:
            # Determine differences
            comein = playersNew.difference(self.players)
            goout = self.players.difference(playersNew)
    
            # Set our new list
            self.players = playersNew
            
            # Send a message and update our tooltip
            self.messenger.notify(self.messenger.makeMessage(comein, goout))
            self.guiApp.refreshToolTip(self.players)
            
        # Guarantee a tooltip if no users are online
        if self.firstRun == True:
            self.guiApp.refreshToolTip(self.players)
            self.firstRun = False
            
            
        # Must return true to continue the periodic timer
        return True;


"""
GuiApplicationBase
 
This handles all GUI (system tray) related functions and set up.
"""
class GuiApplicationBase(object):
    def __init__(self, iconPath):
        self.iconPath = iconPath
        
        self.title = GUI_TITLE
        self.timer = None
            
            
    # Override these methods in sub-classes
    def refreshToolTip(self, players):
        logging.warn("Override me: %s", inspect.stack()[0][3])

    
    # *args is needed here for some callback calls (like with gtk+)
    def launchExecutable(self, *args):
        logging.warn("Override me: %s", inspect.stack()[0][3])
        

    # Common methods
    def setTimer(self, timer):
        self.timer = timer

        
    def formTooltip(self, players):
        if len(players) > 0:
            verb = 'is'
            if len(players) > 1:
                verb = 'are'
                
            return "{0}\n{1} online".format(", ".join(players), verb)
        else:
            return "Nobody is online"


"""   
MessengerBase
 
This sends a message to the GUI.
"""
class MessengerBase(object):
    def __init__(self, timeout):
        self.timeout = timeout
        self.title = MESSAGE_TITLE
            
    # Common methods
    def notify(self, message):
        logging.warn("Override me: %s", inspect.stack()[0][3])
        
            
    def makeMessage(self, comein, goout):
        verbIn = verbOut = 'has'
        
        if len(comein) > 1:
            verbIn = 'have'
        if len(goout) > 1:
            verbOut = 'have'
                
        if len(comein) and len(goout):
            body="{0} {1} joined\n{2} {3} left".format(", ".join(comein),verbIn, ", ".join(goout), verbOut)
        elif len(comein):
            body="{0} {1} joined".format(", ".join(comein), verbIn)
        elif len(goout):
            body="{0} {1} left".format(", ".join(goout), verbOut)
        return body


"""
NotifierBase
   
This is main entry point for each port (windows, linux, osx, etc.).
"""
class NotifierBase(object):
    def __init__(self):
        self.url = URL
        self.iconPath = ICON_PATH
        self.appName = APPLICATION_NAME
        self.notificationTimeout = NOTIFICATION_TIMEOUT
        self.refreshInterval = REFRESH_INTERVAL
        self.executable = EXECUTABLE
        
        self.messenger = None
        self.guiApp = None
        
        
    def on_start(self):
        if self.messenger == None:
            logging.error("messenger not set up?")
            return
            
        self.messenger.notify("Bitfighter Notifier Started")
        
        
    # Set up timers
    def initialize(self, messenger, guiApp):
        self.messenger = messenger
        self.guiApp = guiApp
        
        plr = PlayersListReceiver(self.url, self.messenger, self.guiApp)
        
        timer = PeriodicTimer(self.refreshInterval, plr.refresh)
        timer.start()
        
        self.guiApp.setTimer(timer)
        
        # One time timer to notify user we've started up after 1 second
        startTimer = threading.Timer(1, self.on_start)
        startTimer.start()
        
        
    def run(self):
        logging.warn("Override me: %s", inspect.stack()[0][3])
    

'''
Platform-specific implementation classes
'''

# Start Windows
if sys.platform == 'win32':
    from systray import systray
    
    
    class MessengerWindows(MessengerBase):
        def __init__(self, timeout, guiApp):
            super(MessengerWindows, self).__init__(timeout)
            
            self.guiApp = guiApp
    
    
        def notify(self, message):
            self.guiApp.trayapp.systray.show_message(message)
    
    
    class GuiApplicationWindows(GuiApplicationBase):
        def __init__(self, executable, iconPath):
            super(GuiApplicationWindows, self).__init__(iconPath)
            
            self.executable = executable
            self.trayapp = None
            
            
        def getNotificationIcon(self):
            try:
                return None
            except:
                return None
            
            
        def launchExecutable(self, *args):
            try:
                subprocess.Popen(self.executable, shell=False)
                logging.debug("Running Bitfighter with path " + self.executable)
            except:
                logging.warning("Unable to run {0}, trying default path".format(self.executable))
                logging.debug("Attempting default installation path")
                
                if os.path.exists(os.path.join(os.path.expandvars("%ProgramFiles%"), "bitfighter", "bitfighter.exe")):
                    logging.debug("Attempting " + os.path.join(os.path.expandvars("%ProgramFiles%"), "bitfighter", "bitfighter.exe"))
                    subprocess.Popen(os.path.join(os.path.expandvars("%ProgramFiles%"), "bitfighter", "bitfighter.exe"), shell=True)
                elif os.path.exists(os.path.join(os.path.expandvars("%ProgramFiles(x86)%"), "bitfighter", "bitfighter.exe")):
                    logging.debug("Attempting " + os.path.join(os.path.expandvars("%ProgramFiles(x86)%"), "bitfighter", "bitfighter.exe"))
                    subprocess.Popen(os.path.join(os.path.expandvars("%ProgramFiles(x86)%"), "bitfighter", "bitfighter.exe"), shell=True)
                else:
                    logging.error("Unable to find {0} in default paths".format(self.executable))
    
    
        def rightClickEvent(self, icon, button, time):
            pass
    
    
        def refreshToolTip(self, players):
            if self.trayapp is None:
                return
            
            self.trayapp.systray.set_status("\n" + self.formTooltip(players))
    
            
        def onQuit(self, _systrayApp):
            if self.timer:
                self.timer.cancel()
    
    
        def run(self):
            self.trayapp = systray.App(self.title, self.iconPath)
            self.trayapp.on_quit = self.onQuit
            self.trayapp.on_double_click = self.launchExecutable
    
            self.trayapp.start()
    
    
    class NotifierWindows(NotifierBase):
        def __init__(self):
            super(NotifierWindows, self).__init__()
            logging.debug("Windows!")
    
    
        def run(self):
            guiApp = GuiApplicationWindows(self.executable, self.iconPath)
            messenger = MessengerWindows(self.notificationTimeout, guiApp)
            
            # Set up timers
            self.initialize(messenger, guiApp)
            
            guiApp.run()    
            
# End Windows


# Start OSX
elif sys.platform == 'darwin':
    import objc
    from Foundation import *
    from AppKit import *
    from PyObjCTools import AppHelper
    import platform
    
    
    """
    This uses a thread-blocking NSAlert.  If the alert is up, the application
    does not further update the online status
    """
    class LegacyAlert(object):
        def __init__(self, messageText):
            super(LegacyAlert, self).__init__()
            self.messageText = messageText
            self.informativeText = ""
            
            
        def closeAlert(self):
            NSApp.abortModal()
        
        
        def displayAlert(self):
            # Create timer to shut down alert after some seconds
            selector = objc.selector(self.closeAlert,signature='v@:')
            timer = NSTimer.timerWithTimeInterval_target_selector_userInfo_repeats_(NOTIFICATION_TIMEOUT, self, selector, None, False)
            
            # Add timer to main run loop
            NSRunLoop.mainRunLoop().addTimer_forMode_(timer, NSModalPanelRunLoopMode)

            # Create alert
            alert = NSAlert.alloc().init()
            alert.setMessageText_(self.messageText)
            alert.setInformativeText_(self.informativeText)
            alert.setAlertStyle_(NSInformationalAlertStyle)
            alert.addButtonWithTitle_("OK")
            NSApp.activateIgnoringOtherApps_(True)

            # Run the alert
            alert.runModal()
            
            timer.invalidate()
            
    
    class MessengerOSXLegacy(NSObject, MessengerBase):
        def __init__(self, timeout):
            MessengerBase.__init__(timeout)
            
        
        def notify(self, message):
            pool = NSAutoreleasePool.alloc().init()
            
            ap = LegacyAlert(self.title)
            ap.informativeText = message
            ap.displayAlert()
            
            del pool
            
        def setMembers(self, timeout, title):
            self.timeout = timeout
            self.title = title
            
    
    class MessengerOSXMountainLion(NSObject, MessengerBase):
        def __init__(self, timeout):
            MessengerBase.__init__(timeout)
            
    
        def notify(self, message):
            # We need to set up our own autorelease pool because this happens outside of the main 
            # thread where there is no pool
            pool = NSAutoreleasePool.alloc().init()
            
            NSUserNotification = objc.lookUpClass('NSUserNotification')
            NSUserNotificationCenter = objc.lookUpClass('NSUserNotificationCenter')
            notification = NSUserNotification.alloc().init()
            notification.setTitle_(self.title)
            notification.setInformativeText_(message)
            NSUserNotificationCenter.defaultUserNotificationCenter().setDelegate_(self)
            NSUserNotificationCenter.defaultUserNotificationCenter().scheduleNotification_(notification)
            
            del pool
            
            
        def setMembers(self, timeout, title):
            self.timeout = timeout
            self.title = title
            
    
    class GuiApplicationOSX(NSObject, GuiApplicationBase):
        statusbar = None
        state = 'idle'
        statusitem = None
    
        
        def setMembers(self, executable, iconPath):
            self.iconPath = iconPath
                
                
        def setTimer(self, timer):
            self.timer = timer
            
        
        def applicationDidFinishLaunching_(self, notification):
            statusbar = NSStatusBar.systemStatusBar()
            self.statusitem = statusbar.statusItemWithLength_(NSVariableStatusItemLength)
            
            try:
                self.statusitem.setImage_(NSImage.alloc().initByReferencingFile_(self.iconPath))
            except:
                logging.exception("Unable to load icon, use label instead")
                self.statusitem.setTitle_(self.title)
            
            self.statusitem.setHighlightMode_(1)
    
            # Build a very simple menu
            self.menu = NSMenu.alloc().init()
    
            # Default event
            menuitem = NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Exit', 'terminate:', '')
            self.menu.addItem_(menuitem)
    
            # Bind it to the status item
            self.statusitem.setMenu_(self.menu)
            
            # FIXME: We need to bind the self.timer.cancel() method to a quit callback
            # OSX seems to kill thread anyways...  so..  maybe not needed?
    
    
        def refreshToolTip(self, players):
            if self.statusitem is None:
                return
            
            pool = NSAutoreleasePool.alloc().init()
            
            self.statusitem.setToolTip_(self.formTooltip(players))
            
            del pool
            
            
    class NotifierOSX(NotifierBase):
        def __init__(self):
            super(NotifierOSX, self).__init__()
            logging.debug("OSX!")
            
    
        def run(self):
            
            osxVersion, _, _ = platform.mac_ver()
            majorVersion = int(osxVersion.split('.')[0])
            minorVersion = int(osxVersion.split('.')[1])
            
            logging.debug("major: %s; minor: %s", majorVersion, minorVersion)
            
            app = NSApplication.sharedApplication()
            guiApp = GuiApplicationOSX.alloc().init()
            app.setDelegate_(guiApp)
    
            guiApp.setMembers(self.executable, self.iconPath)
    
            messenger = None
            
            # OSX 10.8 and greater has a built-in notification system we can take advantage of
            if minorVersion >= 8:
                messenger = MessengerOSXMountainLion.alloc().init()
            else:
                messenger = MessengerOSXLegacy.alloc().init()
            
            # Multiple inheritance not working with NSObject??
            messenger.setMembers(self.notificationTimeout, MESSAGE_TITLE)
            

            self.initialize(messenger, guiApp)
            
            AppHelper.runEventLoop()

# End OSX


# Start Linux
elif sys.platform.startswith('linux'): # or sys.platform.startswith('freebsd'):

    # The hard part. check the runtime environment
    isQtGui = False
    isGIGtk = False
    isPyGtk = False
    
    isNotify2 = False
    isDBus = False
    isCmdNotifySend = False
    
    
    # detect GUI
    def checkQtGui():
        global QApplication, QSystemTrayIcon, QMenu, QIcon, QImage, QObject, SIGNAL, isQtGui
        try:
            from PyQt4.QtGui import QApplication, QSystemTrayIcon, QMenu, QIcon, QImage
            from PyQt4.QtCore import QObject, SIGNAL
            isQtGui = True
        except:
            pass
        return isQtGui
    
    
    def checkGIGtk():
        global gtk, gobject, GdkPixbuf, isGIGtk
        try:
            from gi.repository import Gtk as gtk
            from gi.repository import GObject as gobject
            from gi.repository import GdkPixbuf
            isGIGtk = True
        except:
            pass
        return isGIGtk
    
    
    def checkPyGtk():
        global gtk, gobject, isPyGtk
        try:
            import pygtk
            import gtk
            import gobject
            isPyGtk = True
        except:
            pass
        return isPyGtk
    
    
    # Detect notification engine
    def checkNotify2():
        global notify2, isNotify2
        try:
            import notify2
            isNotify2 = True
        except:
            pass
        return isNotify2
    
    
    def checkDBus():
        global dbus, isDBus
        try:
            import dbus
            isDBus = True
        except:
            pass
        return isDBus
    
    
    def checkCmdNotifySend():
        global isCmdNotifySend
        try:
            isCmdNotifySend = subprocess.call(["type", "notify-send"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True) == 0
        finally:
            return isCmdNotifySend
    
    
    # XXX Change order here for precedence
    guiChecks = [checkQtGui, checkPyGtk, checkGIGtk]
    engineChecks = [checkNotify2, checkDBus, checkCmdNotifySend]
    
    
    for check in guiChecks:
        if check():
            logging.info("using: %s", check.__name__)
            break
    else:
        logging.error("No suitable GUI toolkit found. Install either PyQt4 or GTK+ bindings for Python")
        exit(1)
    
    for check in engineChecks:
        if check():
            logging.info("using: %s", check.__name__)
            break
    else:
        logging.error("No suitable notification interface found. Install Python library notify2 or Python bindings to dbus or notify-send command")
        exit(1)
    
    
    # =============================================================
    # Set up GUI classes depending on what was detected
    
    if isQtGui:
        class GuiApplicationLinux(GuiApplicationBase):
            def __init__(self, executable, iconPath, parent=None):
                super(GuiApplicationLinux, self).__init__(iconPath)
                
                self.eventLoop = 'qt'
                self.app = QApplication(sys.argv) # this should be done before anything else
                self.executable = executable
                
                if QIcon.hasThemeIcon(iconPath):
                    icon = QIcon.fromTheme(iconPath)
                else:
                    icon = QIcon(iconPath)
                    
                self.statusIcon = QSystemTrayIcon(icon, parent)
                self.menu = QMenu(parent)
        
                exitAction = self.menu.addAction("Exit")
                exitAction.triggered.connect(self.quit)
                
                self.statusIcon.setContextMenu(self.menu)
                
                def activate(reason):
                    if reason == QSystemTrayIcon.Trigger:
                        return self.launchExecutable()
    
                QObject.connect(self.statusIcon,
                    SIGNAL("activated(QSystemTrayIcon::ActivationReason)"), activate)
    
                self.statusIcon.show()
                
            
            # A portable icon wrapper. Notify2 demands icon class to be compatible with GdkPixbuf so we 
            # provide a compatibility layer
            class IconWrapper(object):
                def __init__(self, iconName):
                    if QIcon.hasThemeIcon(iconName):
                        icon = QIcon.fromTheme(iconName)
                    else:
                        icon = QIcon(iconName)
                    size = icon.availableSizes()[0]
                    self.image = icon.pixmap(size).toImage().convertToFormat(QImage.Format_ARGB32)
                    self.image = self.image.rgbSwapped() # otherwise colors are weird :/
        
                def get_width(self):
                    return self.image.width()
        
                def get_height(self):
                    return self.image.height()
        
                def get_rowstride(self):
                    return self.image.bytesPerLine()
        
                def get_has_alpha(self):
                    return self.image.hasAlphaChannel()
        
                def get_bits_per_sample(self):
                    return self.image.depth() // self.get_n_channels()
        
                def get_n_channels(self):
                    if self.image.isGrayscale():
                        return 1
                    elif self.image.hasAlphaChannel():
                        return 4
                    else:
                        return 3; 
        
                def get_pixels(self):
                    return self.image.bits().asstring(self.image.numBytes())
            # end of wrapper class
     
        
            def getNotificationIcon(self):
                try:
                    return self.IconWrapper(self.iconPath)
                except:
                    logging.error("Failed to get notification icon")
                    return None
    
    
            def refreshToolTip(self, players):
                self.statusIcon.setToolTip(self.formTooltip(players))
                
                
            def launchExecutable(self, *args):
                try:
                    subprocess.Popen(self.executable, shell=True)
                except:
                    logging.error("Unable to run {0}".format(self.cmd))
    
                
            def run(self):
                sys.exit(self.app.exec_())
    
    
            def quit(self):
                self.timer.cancel()
                sys.exit(0)
    
            
    elif isGIGtk or isPyGtk:
        class GuiApplicationLinux(GuiApplicationBase):
            def __init__(self, executable, iconPath, parent = None):
                super(GuiApplicationLinux, self).__init__(iconPath)
                
                self.eventLoop = 'glib'
                self.executable = executable
                self.statusIcon = gtk.StatusIcon()
                self.statusIcon.set_from_file(iconPath)
                self.statusIcon.connect("activate", self.launchExecutable)
                self.statusIcon.connect("popup-menu", self.rightClickEvent)
                self.statusIcon.set_visible(True)
                
                
            def getNotificationIcon(self):
                try:
                    if isGIGtk:
                        return GdkPixbuf.Pixbuf.new_from_file(self.iconPath)
                    else:
                        return gtk.gdk.pixbuf_new_from_file(self.iconPath)
                except:
                    return None
                
                
            def launchExecutable(self, *args):
                try:
                    subprocess.Popen(self.executable, shell=True)
                except:
                    logging.error("Unable to run {0}".format(self.cmd))
    
    
            def rightClickEvent(self, icon, button, time):
                menu = gtk.Menu()
    
                quitMenu = gtk.MenuItem("Exit")
                quitMenu.connect("activate", gtk.main_quit)
            
                menu.append(quitMenu)
                menu.show_all()
    
                if isGIGtk:
                    menuPosition = gtk.StatusIcon.position_menu
                    menu.popup(None, None, menuPosition, self.statusIcon, button, time)
                else:
                    menuPosition = gtk.status_icon_position_menu
                    menu.popup(None, None, menuPosition, button, time, self.statusIcon)
    
        
            def refreshToolTip(self, players):
                self.statusIcon.set_tooltip_text(self.formTooltip(players))
    
    
            def run(self):
                gobject.threads_init()   # This is needed or our python-based timer will fail
                gtk.quit_add(0, self.quit)
                gtk.main()
                
                
            def quit(self):
                self.timer.cancel()
                sys.exit(0)
    
    # =============================================================
    # Set up messenger classes depending on what was detected
    
    if isNotify2:
        class MessengerLinux(MessengerBase):
            def __init__(self, appName, timeout, guiApp):
                super(MessengerLinux, self).__init__(timeout)
                
                notify2.init(appName, guiApp.eventLoop)
                self.notification = None
                
                if guiApp.iconPath:
                    self.icon = guiApp.getNotificationIcon()
    
    
            def notify(self, message):
                if not self.notification:
                    self.notification = notify2.Notification(self.title, message)
                    if self.icon and self.icon.get_width() > 0:
                        self.notification.set_icon_from_pixbuf(self.icon)
                    self.notification.timeout = self.timeout
                else:
                    self.notification.update(self.title, message)
                self.notification.show()
    
    
    elif isDBus:
        class MessengerLinux(MessengerBase):
            def __init__(self, appName, timeout, guiApp):
                super(MessengerLinux, self).__init__(timeout)
                
                self.appName = appName
                # copied from pynotify2
                if guiApp.eventLoop == 'glib':
                    from dbus.mainloop.glib import DBusGMainLoop
                    self.mainloop = DBusGMainLoop()
                elif guiApp.eventLoop == 'qt':
                    from dbus.mainloop.qt import DBusQtMainLoop
                    self.mainloop = DBusQtMainLoop()
                    
                if guiApp.iconPath:
                    self.icon = guiApp.getNotificationIcon()
    
    
            def notify(self, message):
                item = 'org.freedesktop.Notifications'
                path = '/org/freedesktop/Notifications'
                interface = 'org.freedesktop.Notifications'
                iconName = ''
                actions = []
                hints = {}
                if self.icon and self.icon.get_width() > 0:
                    struct = (
                        self.icon.get_width(),
                        self.icon.get_height(),
                        self.icon.get_rowstride(),
                        self.icon.get_has_alpha(),
                        self.icon.get_bits_per_sample(),
                        self.icon.get_n_channels(),
                        dbus.ByteArray(self.icon.get_pixels())
                    )
                    hints['icon_data'] = struct
    
                bus=dbus.SessionBus(self.mainloop)
                nobj = bus.get_object(item, path)
                notify = dbus.Interface(nobj, interface)
                notify.Notify(self.appName, 0, iconName, self.title, message, actions, hints, self.timeout)
    
    
    elif isCmdNotifySend:
        class MessengerLinux(MessengerBase):
            def __init__(self, appName, timeout, guiApp):
                super(MessengerLinux, self).__init__(timeout)
                
                self.appName = appName
                
                if guiApp.iconPath and os.path.exists(guiApp.iconPath):
                    self.iconPath = os.path.abspath(guiApp.iconPath) # the path must be absolute
                else:
                    self.iconPath = guiApp.iconPath 
    
    
            def notify(self, message):
                args = ["notify-send", "--app-name", self.appName, "--expire-time", str(self.timeout)]
                
                if self.iconPath:
                    args.append("--icon")
                    args.append(self.iconPath)
                args.append(self.title)
                args.append(message)
                
                try:
                    subprocess.call(args,  stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                except:
                    logging.exception("notify-send ({0}) call failed".format(args))
                    
            
    class NotifierLinux(NotifierBase):
        def __init__(self):
            super(NotifierLinux, self).__init__()
            logging.debug("Linux!")
            
    #        self.guiType = None
    #        self.engineType = None
    
    
        def run(self):
    #        self.detectGui()
    #        self.detectEngine()
        
            guiApp = GuiApplicationLinux(self.executable, self.iconPath)
            messenger = MessengerLinux(self.appName, self.notificationTimeout, guiApp)
            
            self.initialize(messenger, guiApp)
            
            guiApp.run()
            
            
        """
        # Attempt to simplify the Linux part into classes/methods...  so far failing
        
        # Try to detect the GUI toolkit to use
        def detectGui(self):
            if self.guiType is None:
                global QApplication, QSystemTrayIcon, QMenu, QIcon, QImage, QTimer, QObject, SIGNAL, isQtGui
                try:
                    from PyQt4.QtGui import QApplication, QSystemTrayIcon, QMenu, QIcon, QImage
                    from PyQt4.QtCore import QTimer, QObject, SIGNAL
                    
                    isQtGui = True
                except:
                    pass
            
            if self.guiType is None:
                global gtk, gobject, GdkPixbuf, isGIGtk
                try:
                    from gi.repository import Gtk as gtk
                    from gi.repository import GObject as gobject
                    from gi.repository import GdkPixbuf
                    
                    isGIGtk = True
                except:
                    pass
                
            if self.guiType is None:
                global gtk, gobject, isPyGtk
                try:
                    import pygtk
                    import gtk
                    import gobject
                    
                    isPyGtk = True
                except:
                    pass
    
            if self.guiType is None:
                logging.error("No GUI found.  Exiting")
                exit(1)
            else:
                logging.info("Detected GUI: %s", self.guiType)
            
            
        # Try to detect the notification engine to use
        def detectEngine(self):
            if self.engineType is None:
                global notify2, isNotify2
                try:
                    import notify2
    
                    isNotify2 = True
                except:
                    pass
            
            if self.engineType is None:
                global dbus
                try:
                    import dbus
                    
                    self.engineType = 'dbus'
                except:
                    pass
            
            if self.engineType is None:
                try:
                    isCmdNotifySend = subprocess.call(["type", "notify-send"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True) == 0
                
                    if isCmdNotifySend:
                        self.engineType = 'notify-send'
                except:
                    pass
                    
            if self.engineType is None:
                logging.error("No notification engine found.  Exiting")
                exit(1)
            else:
                logging.info("Detected engine: %s", self.engineType)
        """
    

# End Linux    

else:
    logging.error("This platform is not currently handled.  Join #bitfighter and complain to the developers")



"""
Main!
"""
def main():
    logging.info("starting up!")
    
    notifier = None
    if sys.platform == 'win32':
        notifier = NotifierWindows()
    elif sys.platform == 'darwin':
        notifier = NotifierOSX()
    elif sys.platform.startswith('linux'):
        notifier = NotifierLinux()
    else:
        logging.error("This platform is not currently handled.  Join #bitfighter on freenode and complain to the developers")

    if len(sys.argv) == 2:
        notifier.executable = sys.argv[1]
        logging.debug("Custom path supplied: " + sys.argv[1])
    
    if notifier != None:
        notifier.run()
    
    
if __name__ == '__main__':
    main()