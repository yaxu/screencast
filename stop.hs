
import Sound.OpenSoundControl
import Network.Netclock.Client
import System.Exit

main = stop
          
stop :: IO ()
stop = do serv <- openUDP "127.0.0.1" 7777
          send serv (Message "/screensave/stop" [])
          return ()
          