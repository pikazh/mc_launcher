
import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.event.entity.EntityJoinWorldEvent;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.PlayerEvent;
import net.minecraftforge.fml.common.network.FMLNetworkEvent;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Created by Will_Zhang on 2016/6/22.
 */
public class CraftBoxEventHandler {

    public static Logger logger = LogManager.getLogger(ModInformation.NAME);

    CraftBoxHelper craftBoxHelper;
    boolean connected = false;
    CraftBoxEventHandler(CraftBoxHelper helper){
        craftBoxHelper = helper;

    }
    @SubscribeEvent
    public void onClientConnectedToServer(FMLNetworkEvent.ClientConnectedToServerEvent event) {
        if(!event.isLocal()){
            connected = true;
        }
    }

    @SubscribeEvent
    public void onClientDisconnectionFromServer(FMLNetworkEvent.ClientDisconnectionFromServerEvent event) {
        craftBoxHelper.disconnectGame();
        connected = false;
    }

    @SubscribeEvent
    public void entityJoinWorld(EntityJoinWorldEvent event) {
        if (event.getEntity() instanceof bmt) {

            if(connected){
                connected = false;
                craftBoxHelper.joinGame();
            }
        }
    }
}
