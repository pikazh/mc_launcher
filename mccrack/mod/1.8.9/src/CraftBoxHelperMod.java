import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.fml.common.FMLCommonHandler;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.event.FMLPreInitializationEvent;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;


@Mod(modid = ModInformation.ID, name = ModInformation.NAME, version = ModInformation.VERSION, dependencies = ModInformation.DEPEND)
public class CraftBoxHelperMod {

    public static Logger logger = LogManager.getLogger(ModInformation.NAME);
    protected CraftBoxHelper craftBoxHelper;

    @Mod.EventHandler
    public void preInit(FMLPreInitializationEvent event) {

        logger.info("info." + ModInformation.ID + ".console.load.preInit");
        craftBoxHelper = new CraftBoxHelper(ave.A());
        MinecraftForge.EVENT_BUS.register(new CraftBoxEventHandler(craftBoxHelper));
        FMLCommonHandler.instance().bus().register(new CraftBoxEventHandler(craftBoxHelper));

    }
}

