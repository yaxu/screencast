
import Sound.OpenSoundControl
import Network.Netclock.Client
import System.Exit

main = do clocked "starter" "127.0.0.1" "127.0.0.1" 2 onTick
          
onTick :: BpsChange -> Int -> IO ()
onTick bps ticks = do putStrLn $ "aha " ++ show ticks
                      if ticks `mod` 8 == 1
                        then start
                        else return ()
start :: IO ()
start = do serv <- openUDP "127.0.0.1" 7777
           send serv (Message "/screensave/start" [])
           exitWith ExitSuccess
          