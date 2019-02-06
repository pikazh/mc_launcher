import cpw.mods.fml.common.eventhandler.SubscribeEvent;
import cpw.mods.fml.common.gameevent.PlayerEvent;
import cpw.mods.fml.common.network.FMLNetworkEvent;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Created by Will_Zhang on 2016/6/22.
 */
public class CraftBoxEventHandler {

    public static Logger logger = LogManager.getLogger(ModInformation.NAME);

    CraftBoxHelper craftBoxHelper;
    CraftBoxEventHandler(CraftBoxHelper helper){
        craftBoxHelper = helper;

    }
    @SubscribeEvent
    public void onClientConnectedToServer(FMLNetworkEvent.ClientConnectedToServerEvent event) {

        // Disable server specific options, which will be re-enabled if the server replies
        if(!event.isLocal){
            craftBoxHelper.joinGame();
        }
    }

    @SubscribeEvent
    public void onClientDisconnectionFromServer(FMLNetworkEvent.ClientDisconnectionFromServerEvent event) {

       craftBoxHelper.disconnectGame();
    }

    @SubscribeEvent
    public void onPlayerLoggedIn(PlayerEvent.PlayerLoggedInEvent event) {
        // Disable server specific options, which will be re-enabled if the server replies

    }
}
