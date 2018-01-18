module TrainSim.Builder (
 withSimulator, withInfrastructureSimulator
) where

import Foreign.Ptr
import Foreign.C.String
import Foreign.C.Types

import Data.Maybe (listToMaybe)
import Control.Monad (forM,forM_)

import TrainSim.ConvertInput
import TrainPlan.Infrastructure (Direction(..),SwitchPosition(..))
import qualified TrainPlan.Infrastructure

data Simulator
data Release
data Route

foreign import ccall unsafe "new_infrastructurespec" new_infrastructure :: IO (Ptr Simulator)
foreign import ccall unsafe "delete_infrastructurespec" delete_infrastructure :: Ptr Simulator -> IO ()

foreign import ccall unsafe "new_release" new_release :: CSize -> IO (Ptr Release)
foreign import ccall unsafe "new_route" new_route :: CString -> CInt -> CDouble -> IO (Ptr Route)
foreign import ccall unsafe "add_release_resource" add_release_resource :: Ptr Release -> CSize -> IO ()
foreign import ccall unsafe "add_release" add_release :: Ptr Route -> Ptr Release -> IO ()
foreign import ccall unsafe "add_route" add_route :: Ptr Simulator -> Ptr Route -> IO ()
foreign import ccall unsafe "add_route_switch" add_route_switch :: Ptr Route -> CSize -> CInt -> IO ()
foreign import ccall unsafe "add_route_tvd" add_route_tvd :: Ptr Route -> CSize -> IO ()

foreign import ccall unsafe "add_signal" add_signal :: Ptr Simulator -> CString -> CInt -> CDouble -> CInt -> CDouble -> CInt -> IO ()
foreign import ccall unsafe "add_detector" add_detector :: Ptr Simulator -> CString -> CInt -> CDouble -> CInt -> CDouble -> CInt -> CInt -> IO ()
foreign import ccall unsafe "add_sight" add_sight :: Ptr Simulator -> CString -> CInt -> CDouble -> CInt -> CDouble -> CSize -> CDouble -> IO ()
foreign import ccall unsafe "add_boundary" add_boundary :: Ptr Simulator -> CString -> CInt -> CDouble -> CInt -> CDouble ->  IO ()
foreign import ccall unsafe "add_stop" add_stop :: Ptr Simulator -> CString -> CInt -> CDouble -> CInt -> CDouble ->  IO ()
foreign import ccall unsafe "add_tvd" add_tvd :: Ptr Simulator -> CString -> IO ()
foreign import ccall unsafe "add_switch" add_switch :: Ptr Simulator -> CString -> CInt -> CDouble -> CInt -> CDouble -> CInt -> CDouble -> CInt -> CDouble -> CInt -> IO ()

dirRepr :: Direction -> CInt
dirRepr Up = 1
dirRepr Down = 0

boolRepr :: Bool -> CInt
boolRepr True = 1
boolRepr False = 0

swStateRepr :: SwitchPosition -> CInt
swStateRepr SwLeft = 0
swStateRepr SwRight = 1
swStateRepr SwUnknown = 1

maybeLinkRepr :: [Link Int] -> (CInt, CDouble)
maybeLinkRepr x = case listToMaybe x of
  Just (a,b) -> (fromIntegral a,realToFrac b)
  Nothing -> ((-1),0.0)

maybeIntRepr :: Maybe Int -> CInt
maybeIntRepr (Just x) = fromIntegral x
maybeIntRepr Nothing = (-1)

withInfrastructureSimulator :: TrainPlan.Infrastructure.Infrastructure -> (Ptr Simulator -> IO ()) -> IO ()
withInfrastructureSimulator is = withSimulator (convert is)

withSimulator :: Infrastructure Int -> (Ptr Simulator -> IO ()) -> IO ()
withSimulator spec f = do
  isptr <- new_infrastructure
  forM_ (isObjs spec) $ \obj -> do
    let (up1,up1d) = maybeLinkRepr (upLinks obj)
    let (down1,down1d) = maybeLinkRepr (downLinks obj)
    case isObjData obj of
      SignalSpec dir -> add_signal isptr nullPtr up1 up1d down1 down1d (dirRepr dir)
      DetectorSpec uptvd downtvd -> add_detector isptr nullPtr up1 up1d down1 down1d 
        (maybeIntRepr uptvd) (maybeIntRepr downtvd)
      SightSpec x k -> add_sight isptr nullPtr up1 up1d down1 down1d 
        (fromIntegral x) (realToFrac k)
      BoundarySpec -> add_boundary isptr nullPtr up1 up1d down1 down1d
      StopSpec -> add_stop isptr nullPtr up1 up1d down1 down1d
      TVDSpec -> add_tvd isptr nullPtr
      SwitchSpec swstate -> do
        let (up2,up2d) = maybeLinkRepr (drop 1 (upLinks obj))
        let (down2,down2d) = maybeLinkRepr (drop 1 (downLinks obj))
        add_switch isptr nullPtr
          up1 up1d up2 up2d 
          down1 down1d down2 down2d 
          (swStateRepr swstate)
  forM_ (isRoutes spec) $ \route -> do
    putStrLn "ADDING ROUTE"
    routeptr <- new_route nullPtr (maybeIntRepr (rsEntrySignal route)) 
                    (realToFrac (tsLength route))
    forM_ (tsTVDs route) $ \t -> add_route_tvd routeptr (fromIntegral t)
    forM_ (tsSwitchPos route) $ \(x,p) -> add_route_switch routeptr (fromIntegral x) (swStateRepr p)
    forM_ (tsReleases route) $ \(trig,res) -> do
      release_ptr <- new_release (fromIntegral trig)
      forM_ res $ \r -> do add_release_resource release_ptr (fromIntegral r)
      add_release routeptr release_ptr
    add_route isptr routeptr
    putStrLn "DONE ADDING ROUTE"

  putStrLn "STARTING PROCESS WITH PTR"
  f isptr
  putStrLn "ENDING PROCESS WITH PTR"
  delete_infrastructure isptr

test = do
  is <- new_infrastructure
  add_signal is nullPtr (-1) 1.0 2 3.5 1
  delete_infrastructure is
