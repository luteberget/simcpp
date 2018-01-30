{-# LANGUAGE ScopedTypeVariables #-}

module TrainSim.ConvertInput (
  Infrastructure(),
  mkInfrastructureObjs,
  mkInfrastructure,
  resolveIds, completeLinks, Link,
  isObjs, isObjData, upLinks, downLinks, ISObjTypeSpec(..), RouteSpec(..),
  isRoutes,
  convert, convertPlan, Plan, PlanItem(..), TrainRunSpec(..),
  toISGraphFormat
  ) where

import Data.Maybe (listToMaybe, fromMaybe, isJust, maybeToList, isNothing, fromJust, catMaybes)
import Data.List (sortBy,nub)
import Data.Ord (comparing)
import Control.Monad.State
import qualified Data.Map as Map
import Data.Map (Map)
import qualified Data.Set as Set
import Data.Set (Set)
import Data.List (intercalate)

import Data.Char (toLower)

import qualified TrainPlan.Infrastructure as Input
import qualified TrainPlan.UsagePattern as UP
import TrainPlan.Infrastructure (routePointRef, isBoundary)
import TrainPlan.Infrastructure (Direction(..), SwitchPosition(..))

data BeginEnd = Begin | End deriving (Show)
-- data SwitchState = SwLeft | SwRight | SwUnknown deriving (Ord, Eq, Show)

type Link id = (id, Double)
data TrainRunSpec id
  = TrainRunSpec
  { trsVehicleParams :: UP.Vehicle
  , trsStartDir :: Direction
  , trsStartAuthorityLength :: Double
  , trsStartLoc :: Link id
  , trsStops :: [Double]
  }  deriving (Ord, Eq, Show)

data ISObjTypeSpec id
  = SignalSpec Direction id
  | DetectorSpec (Maybe id) (Maybe id) -- tvd up tvd down
  | SightSpec id Double -- Sight point to signal
  | SwitchSpec SwitchPosition -- one or two connections + default state
  | BoundarySpec 
  | StopSpec 
  | TVDSpec
  deriving (Ord, Eq, Show)

data ISObjSpec id 
  = ISObjSpec  
  { isObjId :: id
  , isObjName :: String
  , isObjData :: ISObjTypeSpec id
  , upLinks :: [Link id]
  , downLinks :: [Link id]
  } 
  deriving (Ord, Eq, Show)

type ReleaseSpec id = (id, [id]) -- Trigger and released resources

data RouteSpec id
  = RouteSpec
  { rsEntrySignal :: Maybe id 
  , tsTVDs :: [id]
  , tsSwitchPos :: [(id, SwitchPosition)]
  , tsReleases :: [ReleaseSpec id]
  , tsLength :: Double
  } deriving (Ord, Eq, Show)

data Infrastructure id
 = Infrastructure
 { isObjs   :: [ISObjSpec id]
 , isRoutes :: [(id, RouteSpec id)]
 } deriving (Ord, Eq, Show)

type Plan = [PlanItem]
data PlanItem = PlanStartTrain (TrainRunSpec Int)
              | PlanActivateRoute Int
  deriving (Show, Ord, Eq)

--  , trsStartDir :: Direction
--  , trsStartAuthorityLength :: Double
--  , trsStartLoc :: Link id
--  , trsStops :: [Double]

convertTrain :: UP.UsagePattern -> Int -> Input.Route -> Map String Int -> PlanItem
convertTrain up trainIdx route objmap = PlanStartTrain (TrainRunSpec vehicle dir authLen loc [])
  where
    m = (UP.movements up) !! trainIdx
    vehicle = head [ v | v <- UP.vehicles up, UP.vehicleRef m == UP.vehicleName v ]
    dir = Input.routeDir route
    authLen = Input.length route
    loc = ((objmap Map.! (Input.routePointRef (Input.entry route))), 0.0)

convertPlan :: Input.Infrastructure -> UP.UsagePattern -> [[(Int, Maybe Int)]] -> Map String Int -> Plan
convertPlan is up inplan objmap = join (fmap diff (succPairs (empty:inplan)))
  where
    numberedRoutes = zip [0..] (Input.routes is)
    boundaryEntryRoutes = Set.fromList $ fmap fst (filter (\(i,r) -> isBoundary (Input.entry r)) numberedRoutes)
    empty = [ (i,Nothing) | (i,s1) <- inplan !! 0 ]
    diff :: ([(Int,Maybe Int)], [(Int, Maybe Int)]) -> Plan
    diff (s1,s2) = catMaybes (fmap diffItems (zip s1 s2))
      where
        diffItems ((r1,t1),(r2,t2)) 
          | not (isNothing t2) && t1 /= t2 = Just (go r2 (fromJust t2))
          | otherwise = Nothing
        go :: Int -> Int -> PlanItem
        go r t
          | Set.member r boundaryEntryRoutes = convertTrain up t ((Input.routes is) !! r) objmap
          | otherwise = PlanActivateRoute r

-- TODO missing Plan train entry 

convertRoutes :: Map String Int -> Input.Infrastructure -> [(Int,RouteSpec Int)]
convertRoutes objMap is = (fmap convertInternal internalRoutes) ++ 
                   (fmap convertBoundaryExit boundaryExitRoutes)
  where
    numberedRoutes = zip [0..] (Input.routes is)
    internalRoutes = filter (\(i,r) -> (not (isBoundary (Input.entry r))) && 
                                       (not (isBoundary (Input.exit r)))) numberedRoutes 
    boundaryExitRoutes = filter (\(i,r) -> isBoundary (Input.exit r)) numberedRoutes

    convertInternal (i,r) = (i, RouteSpec (Just e) tvds swpos releases length)
      where 
        e = objMap Map.! (routePointRef (Input.entry r))
        tvds = fmap (\x -> objMap Map.! x) (Input.tvds r)
        swpos = fmap (\(x,p) -> (objMap Map.! x, p)) (Input.switchPos r)
        releases = fmap convertRelease (Input.releases r)
        length = Input.length r
    convertBoundaryExit (i,r) = (i,RouteSpec (Just e) [] [] [] length)
      where
        e = objMap Map.! (routePointRef (Input.entry r))
        length = (read "Infinity" :: Double)

    convertRelease r = (objMap Map.! (Input.trigger r), 
                        fmap (\x -> objMap Map.! x) (Input.resources r))

lengthPrefixedList :: [a] -> (a -> String) -> String
lengthPrefixedList xs f = intercalate " " ((show (length xs)): [ f x | x <- xs ])

toISGraphFormat :: (Monad m) => Input.Infrastructure -> (String -> m ()) -> m ()
toISGraphFormat input f = sequence_ ([ f s | s <- fmap toString objs ] ++
                                     [ f r | r <- join (fmap routeString (filter signalRoutes (Input.routes input))) ])
  where 
    objs = completeLinks singlyLinkedObjs
    (objMap,singlyLinkedObjs) = resolveIds (mkInfrastructureObjs input)

    signalRoutes :: Input.Route -> Bool
    signalRoutes r = case Input.entry r of
        Input.RoutePointSignal _ -> True
        _ -> False
    

    toString :: ISObjSpec Int -> String
    toString (ISObjSpec id name dat up down) = "obj " ++ name ++ " "++  (links up) ++ " " ++ (links down) ++ " " ++ (specToString dat)
    specToString (SignalSpec dir ref) = "signal " ++ (fmap toLower (show dir)) ++ " " ++ (show ref)
    specToString (DetectorSpec a b) = "detector" ++ (maybeWordRepr a) ++ (maybeWordRepr b)
    specToString (SightSpec i d) = "sight " ++ (show i) ++ " " ++ (show d)
    specToString (SwitchSpec pos) = "switch " ++ (show pos)
    specToString (BoundarySpec) = "boundary"
    specToString (StopSpec) = "stop"
    specToString (TVDSpec) = "tvd"

    maybeWordRepr :: Maybe Int -> String
    maybeWordRepr Nothing = " -1"
    maybeWordRepr (Just x) = " " ++ (show x)

    links :: [Link Int] -> String
    links ls = (show (length ls)) ++ (join (fmap link ls))
      where
        link (id,dist) = " " ++ (show id) ++ " " ++ (show dist)


    routeString :: Input.Route -> [String]
    routeString r = ((aquire r):(map (release (Input.routeName r)) (Input.releases r)))

    aquire :: Input.Route -> String
    aquire r = intercalate " " 
                 ["route", Input.routeName r, reprRoutePoint (Input.entry r),
                  show (Input.length r), 
                  lengthPrefixedList (Input.tvds r) (show .((Map.!) objMap)), 
                  lengthPrefixedList (Input.switchPos r) swposRepr ]
      where
        swposRepr :: (Input.NodeRef, Input.SwitchPosition) -> String
        swposRepr (x,p) = show (objMap Map.! x) ++ " " ++ reprSwitchPosition p

        reprSwitchPosition Input.SwLeft = "left"
        reprSwitchPosition Input.SwRight = "right"

        reprRoutePoint :: Input.RoutePoint -> String
        reprRoutePoint (Input.RoutePointBoundary _) = error "route start in boundary"
        reprRoutePoint (Input.RoutePointSignal s) = show (objMap Map.! s)
        reprRoutePoint (Input.RoutePointTrackEnd) = error "route start in track end"

    release :: String -> Input.ReleaseSpec -> String
    release name (Input.ReleaseSpec trig ress) = intercalate " "
                                      ["release", name, show (objMap Map.! trig),
                                       lengthPrefixedList ress (show . ((Map.!) objMap)) ]

--data ISObjTypeSpec id
--  = SignalSpec Direction id
--  | DetectorSpec (Maybe id) (Maybe id) -- tvd up tvd down
--  | SightSpec id -- Sight point to signal
--  | SwitchSpec SwitchPosition -- one or two connections + default state
--  | BoundarySpec
--  | StopSpec
--  | TVDSpec


convert :: Input.Infrastructure -> (Infrastructure Int, Map String Int)
convert input = (Infrastructure isGraph isRoutes, objMap)
  where
    isRoutes = convertRoutes objMap input
    isGraph = completeLinks singlyLinkedObjs
    (objMap,singlyLinkedObjs) = resolveIds (mkInfrastructureObjs input)

mkInfrastructure :: [ISObjSpec Int] -> [(Int, RouteSpec Int)] -> Infrastructure Int
mkInfrastructure a b = Infrastructure a b

mkRoutes :: forall id. (Eq id, Ord id) => [ISObjSpec id] -> [(id, RouteSpec id)]
mkRoutes objs = undefined
  where
    objmap = Map.fromList [(id,x) | x <- objs, let id = isObjId x]
    -- startPtsMap = Map.fromList [(id,x) | (x,_) <- startPts, let id = isObjId x]
    startPts = [ (o,startDir o) | o@(ISObjSpec _ _ BoundarySpec _ _) <- objs]
            ++ [ (o,startDir o) | o@(ISObjSpec _ _ (SignalSpec _ _) _ _) <- objs]

    objDirNexts :: ISObjSpec id -> Direction -> [Link id]
    objDirNexts o Up = upLinks o
    objDirNexts o Down = downLinks o

    routesFrom :: ISObjSpec id -> Direction -> [RouteSpec id]
    routesFrom obj dir = nexts [] [] 0.0 (objDirNexts obj dir)
      where 
        nexts tvds sws len n = join $ fmap (\(i,l) -> go (objmap Map.! i) tvds sws (l+len)) n
        entry = if isBoundary obj then Nothing else Just (isObjId obj)
        go :: ISObjSpec id -> [id] -> [(id,SwitchPosition)] -> Double -> [RouteSpec id]
        go o tvds sws len 
          | isEndPt o = [RouteSpec entry tvds sws [] len]
          | otherwise = case o of
             (ISObjSpec _ _ (SwitchSpec _) _ _) -> join $ (flip map) (zip [SwLeft, SwRight] (objDirNexts o dir)) $ \(swpos,(lid,ll)) -> go (objmap Map.! lid) tvds ((isObjId o, swpos):sws) (len + ll)
             _ -> nexts tvds sws len (objDirNexts o dir)

        -- addSw :: [(id,SwitchPosition)] -> ISObjSpec id -> [(id,SwitchPosition)]
        -- addSw sw (ISObjSpec id (SwitchSpec _) ups downs) 
        --   | facing = 
        --   | otherwise = 
        --   where facing = (length ups > length downs) == (dir == Up)

        isEndPt o = not (null [() | (o,dir) <- startPts, startDir o == dir ])

--    trSpec (SignalSpec d) = SignalSpec d
--    trSpec (DetectorSpec a b) = DetectorSpec (fmap trid a) (fmap trid b)
--    trSpec (SightSpec a l) = SightSpec (trid a) l
--    trSpec (SwitchSpec s) = SwitchSpec s
--    trSpec BoundarySpec = BoundarySpec
--    trSpec StopSpec = StopSpec
--    trSpec TVDSpec = TVDSpec

    isBoundary :: ISObjSpec id -> Bool
    isBoundary (ISObjSpec _ _ BoundarySpec _ _) = True
    isBoundary _ = False

    startDir :: ISObjSpec id -> Direction
    startDir (ISObjSpec _ _ (BoundarySpec) upl downl) = if null upl then Down else Up
    startDir (ISObjSpec _ _ (SignalSpec d _) _ _) = d

fresh :: State Int String
fresh = do
  i <- get
  modify' (+1)
  return $ "x" ++ (show i)

componentLocation :: Input.Component -> Input.Location
componentLocation (Input.Signal _ (loc,_) _) = loc
componentLocation (Input.Detector _ loc _ _) = loc
componentLocation (Input.Sight loc _ _) = loc

data NodeComponent = NComponent Input.Component | NBoundary String BeginEnd | NStop BeginEnd | NSwitch BeginEnd deriving (Show)

nodeLocation :: Double -> NodeComponent -> Double
nodeLocation _  (NComponent c) = Input.posLength (componentLocation c)
nodeLocation _  (NBoundary _ Begin) = 0.0
nodeLocation _  (NStop Begin) = 0.0
nodeLocation _  (NSwitch Begin) = 0.0
nodeLocation tl (NBoundary _ End) = tl
nodeLocation tl (NStop End) = tl
nodeLocation tl (NSwitch End) = tl

nodeName :: NodeComponent -> Maybe String
nodeName (NComponent (Input.Signal name _ _)) = Just name
nodeName (NComponent (Input.Detector name _ _ _)) = Just name
nodeName (NBoundary name _) = Just name
nodeName _ = Nothing

type ObjRef = String


completeLinks :: [(String, Int, (ISObjTypeSpec Int, [Link Int]))] -> [ISObjSpec Int]
completeLinks input = [ISObjSpec id name typ up down 
                      | ((name, id,(typ,up)),down) <- zip input (inverseLinks input)]

inverseLinks :: [(String, Int, (ISObjTypeSpec Int, [Link Int]))] -> [[Link Int]]
inverseLinks objs = [fromMaybe [] (Map.lookup i rlinks) | (_name,i,_) <- objs]
  where
    rlinks :: Map Int [Link Int]
    rlinks = Map.unionsWith (++) [ Map.singleton li [l]
                                 | (_name, i,(_,up)) <- objs, (li,l) <- fmap (swapLink i) up]

swapLink :: id -> Link id -> (id, Link id)
swapLink i (li,ll) = (li,(i,ll))

resolveIds :: [([String],(ISObjTypeSpec String, [Link String]))] -> (Map String Int, [(String, Int,(ISObjTypeSpec Int, [Link Int]))])
resolveIds inputs = (refmap, fmap tr inputs)
  where
    tr :: ([String], (ISObjTypeSpec String, [Link String])) -> (String, Int, (ISObjTypeSpec Int, [Link Int]))
    tr (i,(typ,links)) = ((i!!0), trh i, (trSpec typ, fmap trLink links))

    refmap :: Map String Int
    refmap = Map.fromList [(s,i) | (i, (ss,_)) <- zip [0..] inputs , s <- ss]

    trLink :: Link String -> Link Int
    trLink (x,l) = (refmap Map.! x, l)
    trh i = trid (i !! 0)
    trid id = refmap Map.! id

    trSpec :: ISObjTypeSpec String -> ISObjTypeSpec Int
    trSpec (SignalSpec d det) = SignalSpec d (trid det)
    trSpec (DetectorSpec a b) = DetectorSpec (fmap trid a) (fmap trid b)
    trSpec (SightSpec a d) = SightSpec (trid a) d
    trSpec (SwitchSpec s) = SwitchSpec s
    trSpec BoundarySpec = BoundarySpec
    trSpec StopSpec = StopSpec
    trSpec TVDSpec = TVDSpec

mkInfrastructureObjs :: Input.Infrastructure -> [([ObjRef],(ISObjTypeSpec ObjRef,[Link ObjRef]))]
mkInfrastructureObjs is = evalState go 0
  where
    go = do
      objs <- sequence [ mkTrackObjs d | d <- trackData]
      let tvds = [([name], (TVDSpec,[])) | (Input.TVD name) <- Input.components is]
      return (tvds ++ (concat objs))

    trackNameMap :: Map String Input.Track
    trackNameMap = Map.fromList [(id,t) | t <- Input.tracks is, let id = Input.trackId t]

    tcomponents t = (tsignals t) ++ (tdetectors t) ++ (tsights t)
    tsights t   = [ s | s@(Input.Sight (Input.Location tref _) _ _) <- Input.components is
                       , tref == (Input.trackId t) ]
    tsignals t   = [ s | s@(Input.Signal _ ((Input.Location tref _),_) _) <- Input.components is
                       , tref == (Input.trackId t) ]
    tdetectors t = [ d | d@(Input.Detector _ (Input.Location tref _) _ _) <- Input.components is
                       , tref == (Input.trackId t) ]

    distToFirstComponent :: Input.Track -> Double
    distToFirstComponent trk 
      | isJust (beginNode trk) = 0.0
      | otherwise =  Input.posLength (componentLocation (head orderedComponents))
      where orderedComponents = sortBy (comparing componentLocation) (tcomponents trk) 

    trackData :: [(Input.Track, [(ObjRef,Double)], [Input.Component])]
    trackData = [(t, nexts t, tcomponents t) | t <- Input.tracks is]
      where
        nexts t = [ (nt, distToFirstComponent (trackNameMap Map.! nt)) 
                  | (Input.Node _ (Input.ConnectionNode from) (Input.ConnectionNode to)) <- Input.nodes is
                  , (Input.trackId t) `elem` from
                  , nt <- to]

    beginNode :: Input.Track -> Maybe NodeComponent
    beginNode track 
      | isJust boundary = Just (NBoundary (fromJust boundary) Begin)
      | stop = Just (NStop Begin)
      | incomingSwitch = Just (NSwitch Begin)
      | otherwise = Nothing
      where
        -- TODO In Node constructor, take out name of boundary to refer to in RunSpec
        -- Boundary node: a boundary node points to this track.
        boundary = listToMaybe [name | (Input.Node name Input.BoundaryNode 
                                                 (Input.ConnectionNode [x]))
                                      <- Input.nodes is, x == Input.trackId track]
        -- Stop node: No nodes (switches) link to this node
        stop = null [() | (Input.Node _ (Input.ConnectionNode _) 
                                        (Input.ConnectionNode to)) <- Input.nodes is
                        , (Input.trackId track) `elem` to]
        incomingSwitch = not (null [() | (Input.Node _ (Input.ConnectionNode _)
                                        (Input.ConnectionNode [x])) 
                                       <- Input.nodes is, x == Input.trackId track])

    endNode :: Input.Track -> Maybe NodeComponent
    endNode track
      | isJust boundary = Just (NBoundary (fromJust boundary) End)
      | stop = Just (NStop End)
      | outgoingSwitch = Just (NSwitch End)
      | otherwise = Nothing
      where
        boundary = listToMaybe [name | (Input.Node name (Input.ConnectionNode [x])
                                                 Input.BoundaryNode)
                                      <- Input.nodes is, x == Input.trackId track]
        stop = null [() | (Input.Node _ (Input.ConnectionNode from)
                                        (Input.ConnectionNode _)) <- Input.nodes is
                        , (Input.trackId track) `elem` from]
        outgoingSwitch = not (null [() | (Input.Node _ (Input.ConnectionNode [x])
                                        (Input.ConnectionNode _)) 
                                       <- Input.nodes is, x == Input.trackId track])

-- data NodeComponent = NComponent Input.Component | NBoundary BeginEnd | NStop BeginEnd | NSwitch BeginEnd deriving (Show)
    nodeToObj :: NodeComponent -> [(ObjRef,Double)] -> (ISObjTypeSpec ObjRef, [Link ObjRef])
    nodeToObj (NComponent (Input.Signal name (_,dir) detector)) [lnk] = (SignalSpec dir detector, [lnk])
    nodeToObj (NComponent (Input.Detector _ _ up down)) [lnk] = (DetectorSpec up down, [lnk])
    nodeToObj (NComponent (Input.Sight _ s d)) [lnk] = (SightSpec s d, [lnk])
    -- TVDs should not appear here (they have no location on the track graph)
    nodeToObj (NBoundary name _) links = (BoundarySpec, links)
    nodeToObj (NStop _) links = (StopSpec, links)
    nodeToObj (NSwitch _) links = (SwitchSpec SwLeft, links) -- TODO configurable default state
    -- inconsistent input_
    nodeToObj c s = error ("Could not convert node to object: " ++ (show c) ++ ", " ++ (show s))

    mkTrackObjs :: (Input.Track, [(ObjRef,Double)], [Input.Component]) 
                    -> State Int [([ObjRef], (ISObjTypeSpec ObjRef, [Link ObjRef]))]
    mkTrackObjs (track,trackNexts,components) = do
      let tnodes = (fmap NComponent components) ++ 
                   (maybeToList (beginNode track)) ++ 
                   (maybeToList (endNode   track))  


      let trackPos = (nodeLocation (Input.trackLength track))
      let orderedNodes = sortBy (comparing trackPos) tnodes

      newIds <- sequence [ case nodeName n of 
                             Nothing -> fresh
                             Just x -> return x  
                         | n <- tail orderedNodes ]

      -- seqNodes :: [(Node, NextIdx, LDiff)]
      let seqNodes = zip3 orderedNodes newIds [ n - p | (p,n) 
                                              <- succPairs (fmap trackPos orderedNodes)]

      let remainingL = (Input.trackLength track) - (trackPos (last orderedNodes))
      -- lastNode :: (Node, [(NextIdx, LDiff)])
      let lastNode = (last orderedNodes, fmap (\(idx,l) -> (idx, l+remainingL)) trackNexts)

      let nodes = (fmap (\(n,i,d) -> (n,[(i,d)])) seqNodes) ++ [lastNode]
      let objects = fmap (\(node,nexts) -> nodeToObj node nexts) nodes

      -- Signals could already have a name, and if it's also the first node on
      -- a track, we need to refer to it by both the track name and the signal name.
      let beginId = ((Input.trackId track):(maybeToList (nodeName (head orderedNodes))))

      let withIndices = zip (beginId:(fmap (\x -> [x]) newIds)) objects

      return withIndices



succPairs :: [a] -> [(a,a)]
succPairs [] = []
succPairs [x] = []
succPairs xs = zip xs (tail xs)

