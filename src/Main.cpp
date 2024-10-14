#define VK_ENABLE_BETA_EXTENSIONS

#include <vulkan/vulkan.hpp>
#include "Utility.hpp"

using namespace Vulkan_Test;

const char* requiredLayers[] = 
{ 
    "VK_LAYER_KHRONOS_validation" 
};
const char* requiredInstanceExtentions[] = 
{ 
    vk::KHRPortabilityEnumerationExtensionName
};
const char* requiredDeviceExtentions[] = 
{
    vk::KHRPortabilitySubsetExtensionName
};

// https://chaosplant.tech/do/vulkan/
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// win
// cd build
// ."C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
// VisualStudioでビルド

// mac
// cd build
// cmake . -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
// make

int main()
{
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "YourApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion11; // Vulkan1.1以降に導入された関数を使うために変更
    
    // これに色々情報を乗っけることで
    // 例えばVulkanの拡張機能をオンにしたりだとかデバッグ情報を表示させるといったことができる
    // これはオプション的なもの
    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.pApplicationInfo = &appInfo;
    instCreateInfo.enabledLayerCount = std::size(requiredLayers);
    instCreateInfo.ppEnabledLayerNames = requiredLayers;

#ifdef VULKAN_TEST_MAC
    instCreateInfo.enabledExtensionCount = std::size(requiredInstanceExtentions);
    instCreateInfo.ppEnabledExtensionNames = requiredInstanceExtentions;
    instCreateInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    // Vulkanインスタンスを作ってから壊すまでの間に色々な処理を書いていく
    // Vulkanを使う場合、全ての機能はこのインスタンスを介して利用する
    vk::UniqueInstance instancePtr = vk::createInstanceUnique(instCreateInfo);
    vk::Instance instance = instancePtr.get();

    // GPUが複数あるなら頼む相手をまず選ぶ必要がある
    // GPUの型式などによってサポートしている機能とサポートしていない機能があったりするため、
    // だいたい「インスタンスを介して物理デバイスを列挙する」→「それぞれの物理デバイスの情報を取得する」→「一番いいのを頼む」という流れになる
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    for (vk::PhysicalDevice physicalDevice : physicalDevices)
    {
        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("----------------------------------------");
        LOG(props.properties.deviceName);
    }

    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    uint32_t graphicsQueueFamilyIndex;
    for (size_t i = 0; i < physicalDevices.size(); i++)
    {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();
        bool existsGraphicsQueue = false;

        for (size_t j = 0; j < queueProps.size(); j++)
        {
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                existsGraphicsQueue = true;
                graphicsQueueFamilyIndex = j;
                break;
            }
        }

        if (existsGraphicsQueue)
        {
            physicalDevice = physicalDevices[i];
            existsSuitablePhysicalDevice = true;
            break;
        }
    }

    if (!existsSuitablePhysicalDevice)
    {
        std::cerr << "No physical devices are available." << std::endl;
        return EXIT_FAILURE;
    }

    // Vulkanにおけるキューとは、GPUの実行するコマンドを保持する待ち行列 = GPUのやることリスト
    // GPUにコマンドを送るときは、このキューにコマンドを詰め込むことになる
    // 
    // 1つのGPUが持っているキューは1つだけとは限らない
    // キューによってサポートしている機能とサポートしていない機能がある
    // キューにコマンドを送るときは、そのキューが何の機能をサポートしているかを事前に把握しておく必要がある
    // 
    // ある物理デバイスの持っているキューの情報は getQueueFamilyProperties メソッドで取得できる
    // メソッド名にある「キューファミリ」というのは、同じ能力を持っているキューをひとまとめにしたもの
    // 1つの物理デバイスには1個以上のキューファミリがあり、1つのキューファミリには1個以上の同等の機能を持ったキューが所属している
    // 1つのキューファミリの情報は vk::QueueFamilyProperties 構造体で表される
    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    LOG("----------------------------------------");
    LOG("queue family count: " << queueProps.size());
    for (size_t i = 0; i < queueProps.size(); i++)
    {
        LOG("----------------------------------------");
        LOG("queue family index: " << i);
        LOG("queue count: " << queueProps[i].queueCount);
        LOG("graphic support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics ? "True" : "False"));
        LOG("compute support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute ? "True" : "False"));
        LOG("transfer support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer ? "True" : "False"));
    }

    // Vulkanでは論理デバイスからキューを取得するとき、論理デバイスにあらかじめ「このキューを使うよ」ということを伝えておく必要がある
    // 論理デバイスの作成時、vk::DeviceCreateInfo構造体に「この論理デバイスを通じてこのキューファミリからいくつのキューを使う」という情報を指定する必要がある
    // その情報は vk::DeviceQueueCreateInfo 構造体の配列で表される
    // 
    // 今のところ欲しいキューは1つだけなので要素数1の配列にする
    // queueFamilyIndex はキューの欲しいキューファミリのインデックスを表し、 
    // queueCount はそのキューファミリからいくつのキューが欲しいかを表す
    // 今欲しいのはグラフィック機能をサポートするキューを1つだけ
    // pQueuePriorities はキューのタスク実行の優先度を表すfloat値配列を指定
    // 優先度の値はキューごとに決められるため、この配列はそのキューファミリから欲しいキューの数だけの要素数を持ち
    float queuePriorities[1] = { 1.0f };
    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].pQueuePriorities = queuePriorities;
    queueCreateInfo[0].queueCount = std::size(queueCreateInfo);

    // GPUが複数あるなら頼む相手をまず選ぶ
    // ここで言う「選ぶ」とは、特定のGPUを完全に占有してしまうとかそういう話ではない
    // 
    // 物理デバイスを選んだら次は論理デバイスを作成する
    // VulkanにおいてGPUの能力を使うような機能は、全てこの論理デバイスを通して利用する
    // GPUの能力を使う際、物理デバイスを直接いじることは出来ない
    // 
    // コンピュータの仕組みについて多少知識のある人であれば、
    // この「物理」「論理」の区別はメモリアドレスの「物理アドレス」「論理アドレス」と同じ話であることが想像できる
    // マルチタスクOS上で、1つのプロセスが特定のGPU(=物理デバイス)を独占して管理するような状態はよろしくない
    // 
    // 仮想化されたデバイスが論理デバイス
    // これならあるプロセスが他のプロセスの存在を意識することなくGPUの能力を使うことができる
    
    // インスタンスを作成するときにvk::InstanceCreateInfo構造体を使ったのと同じように、
    // 論理デバイス作成時にもvk::DeviceCreateInfo構造体の中に色々な情報を含めることができる
    vk::DeviceCreateInfo devCreateInfo;
    devCreateInfo.enabledLayerCount = std::size(requiredLayers);
    devCreateInfo.ppEnabledLayerNames = requiredLayers;
    devCreateInfo.queueCreateInfoCount = std::size(queueCreateInfo);
    devCreateInfo.pQueueCreateInfos = queueCreateInfo;

#ifdef VULKAN_TEST_MAC
    devCreateInfo.enabledExtensionCount = std::size(requiredDeviceExtentions);
    devCreateInfo.ppEnabledExtensionNames = requiredDeviceExtentions;
#endif

    // vk::Instanceにはそれに対応するvk::UniqueInstanceが存在しましたが、
    // vk::PhysicalDeviceに対応するvk::UniquePhysicalDevice は存在しない
    // destroyなどを呼ぶ必要もない
    // vk::PhysicalDeviceは単に物理的なデバイスの情報を表しているに過ぎないので、構築したり破棄したりする必要がある類のオブジェクトではない
    vk::UniqueDevice devicePtr = physicalDevice.createDeviceUnique(devCreateInfo);
    vk::Device device = devicePtr.get();

    // キューは論理デバイス(vk::Device)の getQueue メソッドで取得できる
    // 第1引数がキューファミリのインデックス、第2引数が取得したいキューのキューファミリ内でのインデックス
    vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    // これから行う描画処理はメモリの中の話とはいえ、絵を描くキャンバスにあたるものが必要
    // とりあえず幅と高さを決める
    const uint32_t screenWidth = 640;
    const uint32_t screenHeight = 480;

    // Vulkanにおいて画像は vk::Image というオブジェクトで表される
    // imageType には画像の次元を指定する
    // 1次元から3次元まで指定できる
    // 2次元以外の画像は、Rampテクスチャやvoxelなどで使用することがある
    // 
    // extent には画像のサイズを指定する
    // ここでは640×480を指定(１は奥行き)
    // 
    // format には画素のフォーマットの種類を指定
    // どのように各ピクセルが色を保存しているかの情報
    // いろいろなフォーマットが存在するが、今回はひとまずR8G8B8A8Unormという形式を指定する
    // RGB + A(透明度)をこの順でそれぞれ8bitで保存する形式
    // 
    // これで画像オブジェクトを作成することができた
    // しかしこの段階ではまだ画像にメモリが割り当てられていない
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkImageCreateInfo
    vk::ImageCreateInfo imgCreateInfo;
    imgCreateInfo.imageType = vk::ImageType::e2D;
    imgCreateInfo.extent = vk::Extent3D(screenWidth, screenHeight, 1);
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    imgCreateInfo.tiling = vk::ImageTiling::eLinear;
    imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
    imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    vk::UniqueImage image = device.createImageUnique(imgCreateInfo);
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    // コマンドバッファを作るには、その前段階として「コマンドプール」というまた別のオブジェクトを作る必要がある
    // コマンドバッファをコマンドの記録に使うオブジェクトとすれば、コマンドプールというのはコマンドを記録するためのメモリ実体
    // コマンドバッファを作る際には必ず必要
    // コマンドプールは論理デバイス(vk::Device)の createCommandPool メソッド、コマンドバッファは論理デバイス(vk::Device)の allocateCommandBuffersメソッドで作成することができる
    // コマンドプールの作成が「create」なのに対し、コマンドバッファの作成は「allocate」であるあたりから
    // 「コマンドバッファの記録能力は既に用意したコマンドプールから割り当てる」という気持ちが読み取れる
    // 
    // CommandPoolCreateInfoのqueueFamilyIndexには、後でそのコマンドバッファを送信するときに対象とするキューを指定します。
    // 結局送信するときにも「このコマンドバッファをこのキューに送信する」というのは指定するのですが、
    // そこはあまり深く突っ込まないでください。こんな二度手間が盛り沢山なのがVulkanです。
    // 次はコマンドバッファを作成します。
    // allocateCommandBufferではなくallocateCommandBuffers である名前から分かる通り、一度にいくつも作れる仕様になっている
    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    vk::UniqueCommandPool cmdPool = device.createCommandPoolUnique(cmdPoolCreateInfo);

    // コマンドバッファとは、コマンドをため込んでおくバッファ
    // VulkanでGPUに仕事をさせる際は「コマンドバッファの中にコマンドを記録し、そのコマンドバッファをキューに送る」必要がある
    // 
    // 作るコマンドバッファの数はCommandBufferAllocateInfoの commandBufferCount で指定する
    // commandPoolにはコマンドバッファの作成に使うコマンドプールを指定する
    // このコードではUniqueCommandPoolを使っているので.get()を呼び出して生のCommandPoolを取得している
    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(cmdBufAllocInfo);
    vk::MemoryRequirements imgMemReq = device.getImageMemoryRequirements(image.get());

    // コマンドを記録
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBufs[0]->begin(cmdBeginInfo);

    cmdBufs[0]->end();

    // コマンドの記録が終わったらそれをキューに送信
    // vk::SubmitInfo構造体にコマンドバッファの送信に関わる情報を指定する
    // commandBufferCountには送信するコマンドバッファの数
    // pCommandBuffersには送信するコマンドバッファの配列へのポインタを指定
    vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = std::size(submitCmdBuf);
    submitInfo.pCommandBuffers = submitCmdBuf;

    // 送信はvk::Queueの submit メソッドで行う
    // 第2引数は現在nullptrにしているが、本来はフェンスというものを指定することができる
    // こうしてsubmitでGPUに命令を送ることが出来るが、あくまで「送る」だけである
    // キューに仕事を詰め込むだけであるため、submitから処理が返ってきた段階で送った命令が完了されているとは限らない
    std::vector<vk::SubmitInfo> submits;
    submits.push_back(submitInfo);
    graphicsQueue.submit(submits, nullptr);

    // 実用的なプログラムを作るためには、「依頼した処理が終わるまで待つ」方法を知る必要がある
    // これに必要なのが「セマフォ」や「フェンス」
    // とりあえずここでは、単純に「キューが空になるまで待つ」方法を書く
    // 
    // キューのwaitIdleメソッドを呼ぶと、その段階でキューに入っているコマンドが全て実行完了してキューが空になるまで待つことができる
    graphicsQueue.waitIdle();

    return EXIT_SUCCESS;
}

void WaitPrograms(vk::Device device, vk::Queue graphicsQueue, std::vector<vk::SubmitInfo> submits, vk::SubmitInfo submitInfo)
{
    // セマフォなどを使うと送った個別のコマンドに関する待ち処理などが出来る
    // GPUに投げた処理は基本的に非同期処理になる
    // そのため、レンダリングなどの処理をGPUに投げた場合、状況に応じて処理の完了を待つ必要がある
    // このための機構が「フェンス」や「セマフォ」である
    // これらの機構を適切に利用することで、処理の順序関係がおかしなことにならないで済む
    // 
    // フェンスもセマフォもGPUの処理を待つための機構であることに違いない
    // フェンスがホスト側で待機するための機構
    // セマフォは主にGPU内で他の処理を待機するための機構
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 
    // フェンス(ホスト側で待機)
    // 論理デバイスのcreateFenceメソッドでフェンスオブジェクトを作成できる
    // フェンスオブジェクトは内部的に「シグナル状態」と「非シグナル状態」の2状態を持っている
    // 初期状態(あるいはリセット直後の状態)は非シグナル状態
    // 
    // Vulkanでは様々な処理に対して「この処理が終わったらこのフェンスをシグナル状態にしてね」と設定することができ、
    // 論理デバイスのwaitForFencesメソッドでシグナル状態になるまで待機することができる
    // シグナル状態になったフェンスは、resetFencesメソッドで非シグナル状態にリセットできる
    // 例えると、非シグナル状態は赤信号、シグナル状態は青信号
    vk::FenceCreateInfo fenceCreateInfo;
    vk::UniqueFence fence = device.createFenceUnique(fenceCreateInfo);

    // 特定の処理に対して「終わったらシグナル状態にするフェンス」を設定する方法
    // GPUを利用するほとんどの関数は引数としてフェンスを渡せる
    // 例として、キューのsubmitメソッドの第二引数にフェンスを渡すことができる
    // waitForFencesで待機するときはこのように書く
    graphicsQueue.submit(submits, fence.get());
    std::vector<vk::Fence> fences;
    fences.push_back(fence.get());

    // 第一引数に待機するフェンスを設定できる
    //  複数指定することができる
    // 第二引数は待機方法を指定する
    //  TRUEの場合、「全てのフェンスがシグナル状態になるまで待機」となる
    //  FALSEの場合、「どれか一つのフェンスがシグナル状態になるまで待機」となる
    // 第三引数は最大待機時間
    //  ナノ秒で指定
    //  1秒 = 10 ^ 9ナノ秒
    //  Vulkanで時間を指定する場合はナノ秒で指定することが多いので覚えておくと良い
    //  最大待機時間を超えて処理が終わらなかった場合、vk::Result::eTimeoutが返ってくる
    //  本シリーズではあまり真面目に対処していないが、ちゃんとしたアプリケーションを作る場合は考慮すると良い
    //  
    //  正常に終わった場合はvk::Result::eSuccessが返ってくる
    //  resetFencesで非シグナル状態に戻る
    //  これで再びフェンスが使えるようになる
    //  処理開始の直前、もしくは処理待ちの終了直後などにリセットすると良い
    vk::Result result = device.waitForFences(fences, VK_TRUE, 1'000'000'000);
    // これでGPUに投げた処理を待機できる

    // フェンスを初期状態でシグナル状態にしておきたい場合はvk::FenceCreateInfo::flagsにvk::FenceCreateFlagBits::eSignaledを設定する
    // vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    fence = device.createFenceUnique(fenceCreateInfo);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 
    // セマフォ(GPU内で他の処理を待機するための機構)
    // 作の基本はフェンスとそこまで変わらない
    // こちらもシグナル状態と非シグナル状態の2状態があり、シグナル状態になるまで待機することができる
    // タイムラインセマフォという、数値カウンターを持つ特殊で高機能なセマフォもある
    // Vulkanでは2状態のバイナリーセマフォの方が基本的
    // 論理デバイスのcreateSemaphoreメソッドでセマフォオブジェクトを作成できる
    // 
    // フェンスと違い、特定の処理の完了に引っ掛けるには構造体に指定することが多い(例外もあり)
    // 例としてコマンドバッファの送信時は、vk::SubmitInfo構造体のsignalSemaphoreCount、pSignalSemaphoresを用いる
    // ここに指定すると、送信したコマンドの処理が完了した時に指定したセマフォがシグナル状態になる
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    vk::UniqueSemaphore semaphore = device.createSemaphoreUnique(semaphoreCreateInfo);

    vk::Semaphore signalSemaphores[] = { semaphore.get() };
    submitInfo.signalSemaphoreCount = std::size(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;
    submits.push_back(submitInfo);
    graphicsQueue.submit(submits);

    // また、待機のために指定する場合も構造体に指定する
    // 最初に説明したように、セマフォはGPU内で他の処理を待機するために使われることに注意
    // コマンドバッファ送信時は、vk::SubmitInfo構造体のwaitSemaphoreCount、pWaitSemaphores、pWaitDstStageMaskで待機するセマフォを指定する
    vk::Semaphore waitSemaphores[] = { semaphore.get() };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eAllCommands };

    // pWaitDstStageMaskはパイプラインのどの時点で待機するかを指定するもの
    // 単純に処理の開始時点で待機するということもでき、これでも特に動作に問題はない
    // しかし例えば、フラグメントシェーダでだけ特定の処理の結果を待つ必要があるならば、
    // 「フラグメントシェーダの処理が始まる時点で待機」ということにして
    // 頂点シェーダの処理などはシグナル状態を待たずに始める、といったことができる
    // GPUの並列化能力に余裕がある場合はこのように、必要最低限の部分でだけ処理待ちをする方が効率が上がる
    submitInfo.waitSemaphoreCount = std::size(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // セマフォはフェンスと違い、明示的にリセットする必要はない
    // そのセマフォのシグナル状態を待っていた処理が開始すると、自動でリセットされる
    // こうした動作をするので、ある1つの処理に複数の他の処理が依存するという場合はその数だけセマフォが必要
    // 1つのセマフォのシグナルを複数の処理で待つことは一般的ではないため、フェンスのように信号機に例えて考えるのは適切ではない
    // セマフォとは「ある1つの処理の完了」と「ある1つの処理の開始」の順序関係の取り決めであり、1対1の関係を表す
    // 
    // 同期のための機構は実は他にもいろいろある
    // パイプラインバリアやイベント、また本編で扱ったレンダーパスもその1つ
    // しかしフェンスとセマフォが一番基本的で分かりやすく、また使う機会も多い
}