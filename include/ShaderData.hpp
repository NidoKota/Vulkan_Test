#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;
};

struct Vertex {
    Vec2 pos;
    Vec3 color;
};

std::vector<Vertex> vertices = {
    Vertex{ Vec2{-0.5f, -0.5f }, Vec3{ 0.0, 0.0, 1.0 } },
    Vertex{ Vec2{ 0.5f,  0.5f }, Vec3{ 0.0, 1.0, 0.0 } },
    Vertex{ Vec2{-0.5f,  0.5f }, Vec3{ 1.0, 0.0, 0.0 } },
    Vertex{ Vec2{ 0.5f,  0.5f }, Vec3{ 0.0, 1.0, 0.0 } },
    Vertex{ Vec2{-0.5f, -0.5f }, Vec3{ 0.0, 0.0, 1.0 } },
    Vertex{ Vec2{ 0.5f, -0.5f }, Vec3{ 1.0, 1.0, 1.0 } },
};

std::shared_ptr<vk::UniqueBuffer> getVertexBuffer(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    // バッファというのはデバイスメモリ上のデータ列を表すオブジェクト
    // 何度も言うようにGPUから普通のメモリの内容は参照できない
    // シェーダで使いたいデータがある場合、まずデバイスメモリに移す必要がある
    // そしてそれはプログラムの上では「バッファに書き込む」「バッファに書き込んで転送する」という形になる
    // 頂点座標のデータをシェーダに送るためのバッファを作成する
    // いわゆる頂点バッファ
    // size はバッファの大きさをバイト数で示すもの
    // ここに例えば100という値を指定すれば、100バイトの大きさのバッファが作成できる
    // ここでは前節で定義した構造体のバイト数をsizeof演算子で取得し、それにデータの数をかけている
    // usage は作成するバッファの使い道を示すためのもの
    // 今回のように頂点バッファを作る場合、上記のようにvk::BufferUsageFlagBits::eVertexBufferフラグを指定しなければならない
    // 他にも場合によって様々なフラグを指定する必要があり、複数のフラグを指定することもある
    // sharingModeについては今のところは無視
    // ここではvk::SharingMode::eExclusiveを指定
    vk::BufferCreateInfo vertBufferCreateInfo;
    vertBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    vertBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    vertBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    
    *result = device.get().createBufferUnique(vertBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> writeVertexBuffer(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& vertexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    // 上のようにバッファオブジェクトを作成しても、すぐには使えない
    // データを入れるデバイスメモリを別途確保し、バッファオブジェクトと結びつける必要がある
    // 3章のイメージ(vk::Image)のときと同じ
    // イメージに対してgetImageMemoryRequirementsがあったように、バッファにも getBufferMemoryRequirements という関数があり、これで必要なデバイスメモリのタイプ・サイズを取得する
    // その後の流れもイメージのときと同じで、allocateMemoryを用いてメモリを確保する
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements vertexBufMemReq = device.get().getBufferMemoryRequirements(vertexBuf.get());

    vk::MemoryAllocateInfo vertexBufMemAllocInfo;
    vertexBufMemAllocInfo.allocationSize = vertexBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        // 重要なこととして、デバイスメモリの中にはホスト側から書き込めるメモリと書き込めないメモリがある
        // 今回はデータを書き込みたいわけなので、ホスト側から書き込めるようなメモリを選ぶ必要がある
        // ホスト側から見えるようなメモリ(HostVisible)かどうかのチェックを追加
        if (vertexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) 
        {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        exit(EXIT_FAILURE);
    }

    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);

    // デバイスメモリが確保出来たら bindBufferMemoryで結び付ける
    // これもイメージの作成のときと同じような感じ
    // 第1引数は結びつけるバッファ、第2引数は結びつけるデバイスメモリ
    // 第3引数は、これも以前説明したbindImageMemoryと同じ要領で、確保したデバイスメモリのどこを(先頭から何バイト目以降を)使用するかを指定するもの
    device.get().bindBufferMemory(vertexBuf.get(), result->get(), 0);

    // 具体的にデバイスメモリに書き込む方法だが、メモリマッピングというものを行う
    // これは、操作したい対象のデバイスメモリを仮想的にアプリケーションのメモリ空間に対応付けることで操作出来るようにするもの
    // 対象のデバイスメモリを直接操作するわけにはいかないのでこういう形になっている
    //
    // 第1引数はメモリマッピングを行う対象のデバイスメモリ(vk::DeviceMemory)です。
    // 第2、第3引数にはメモリマッピングを行う範囲を指定します。第2引数が範囲の開始地点(先頭から何バイト目)、
    // 第3引数が範囲の大きさ(バイト数)です。今回は第2引数に0、第3引数にデバイスメモリの大きさと同じ値を示しているので、確保したメモリの最初から最後までをマッピングしている
    // 戻り値でマッピングが行われたメモリのアドレスが返ってくるので、この戻り値のアドレスに対して色々操作を行うことでデータを書き込む(読み込む)ことができる
    void* vertexBufMem = device.get().mapMemory(result->get(), 0, sizeof(Vertex) * vertices.size());

    // どんな方法をとっても特に問題はないが、ここではmemcpyを使う
    // 頂点のデータをメモリにコピーする
    std::memcpy(vertexBufMem, vertices.data(), sizeof(Vertex) * vertices.size());

    // 書き込んだら flushMappedMemoryRangesメソッドを呼ぶことで書き込んだ内容がデバイスメモリに反映する
    // マッピングされたメモリはあくまで仮想的にデバイスメモリと対応付けられているだけで、「同期しておけよ」と念をおさないとデータが同期されない可能性がある
    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = result->get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(Vertex) * vertices.size();
    
    // 同期を行う対象と範囲はvk::MappedMemoryRangeで表され、複数指定できる (今回は1つだけ)
    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    // 作業が終わった後はunmapMemoryできちんと後片付けをします。
    device.get().unmapMemory(result->get());
    
    return result;
}

std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> getVertexBindingDescription()
{
    std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> result = std::make_shared<std::vector<vk::VertexInputBindingDescription>>();

    // このままではバッファがどういう形式で頂点のデータを持っているのか、Vulkanのシステム側には分からない
    // Vertex構造体というこちらが勝手に定義してみただけの構造体のことは知りようがない
    // このままではシェーダに読み込ませられない
    // そこで頂点入力デスクリプションというものを用意する
    // 適切な説明(デスクリプション)を与えることで「こういう形式でこういうデータが入っている」という情報を与えることができる
    // データだけでなく説明を与えて初めてシェーダからデータを読み込める

    // 頂点入力デスクリプションには2種類ある
    // アトリビュートデスクリプション(vk::VertexInputAttributeDescription)
    // バインディングデスクリプション(vk::VertexInputBindingDescription)
    // これはひとえに、シェーダに与えるデータを分ける単位が「バインディング」と「アトリビュート」の2段階あるため

    // バインディングが一番上の単位
    // 基本的に1つのバインディングに対して1つのバッファ(の範囲)が結び付けられる
    // あるバインディングに含まれるデータは一定の大きさごとに切り分けられ、各頂点のデータとして解釈される
    // アトリビュートはより細かい単位になる
    // 1つの頂点のデータは、1つまたは複数のアトリビュートとして分けられる
    // 一つの頂点データは「座標」とか「色」みたいな複数のデータを内包していますが、この1つ1つのデータが1つのアトリビュートに相当することになる
    // (注意点として、1つのアトリビュートは「多次元ベクトル」や「行列」だったりできるので、アトリビュートは「x座標」とか「y座標」みたいな単位よりはもう少し大きい単位になる)

    // バインディングデスクリプションは、1つのバインディングのデータをどうやって各頂点のデータに分割するかという情報を設定する。
    // 具体的には、1つの頂点データのバイト数などを設定する。
    // そしてアトリビュートデスクリプションは、1つの頂点データのどの位置からどういう形式でアトリビュートのデータを取り出すかという情報を設定する。
    // アトリビュートデスクリプションはアトリビュートの数だけ作成する。
    // 1つの頂点データが3種類のデータを含んでいるならば、3つのアトリビュートデスクリプションを作らなければならない。
    // 今回のデータだと含んでいるデータは「座標」だけなので1つだけになるが、複数のアトリビュートを含む場合はその数だけ作成する。

    // bindingは説明の対象となるバインディングの番号である。各頂点入力バインディングには0から始まる番号が振ってある。ここでは0番を使うことにする。
    // strideは1つのデータを取り出す際の刻み幅である。つまり上の図でいう所の「1つの頂点データ」の大きさである。ここではVertex構造体のバイト数を指定している。
    // 前節までで用意したデータはVertex構造体が並んでいるわけなので、各データを取り出すには当然Vertex構造体のバイト数ずつずらして読み取ることになる。
    // inputRateにはvk::VertexInputRate::eVertexを指定する。1頂点ごとにデータを読み込む、というだけの意味である。インスタンス化というものを行う場合に別の値を指定する。
    vk::VertexInputBindingDescription vertexBindingDescription;
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(Vertex);
    vertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;

    (*result).push_back(vertexBindingDescription);
    return result;
}

std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> getVertexInputDescription()
{
    std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> result = std::make_shared<std::vector<vk::VertexInputAttributeDescription>>();

    // アトリビュートの読み込み方の説明情報
    // 今度は vk::VertexInputAttributeDescription を用意する
    // 今回用意した頂点データが含む情報は「座標」だけなので、用意するアトリビュートデスクリプションは1つだけ
    //
    // bindingはデータの読み込み元のバインディングの番号を指定。上で0番のバインディングを使うことにしたので、ここでも0にする。
    // locationはシェーダにデータを渡す際のデータ位置。
    //
    // 頂点データの用意に書いたシェーダのコードを見てみる
    // layout(location = 0) in vec2 inPos;
    // location = 0という部分がある。シェーダがアトリビュートのデータを受け取る際はこのようにデータ位置を指定する。
    // このlayout(location = xx)の指定とアトリビュートデスクリプションのlocationの位置は対応付けて書かなければならない。
    // locationの対応付けによって、シェーダで読み込む変数とVulkanが用意したアトリビュートの対応が付くのである。
    //
    // formatはデータ形式である。今回渡すのは32bitのfloat型が2つある2次元ベクトルなので、それを表すeR32G32Sfloatを指定する。
    // ここで使われているvk::Formatは本来ピクセルの色データのフォーマットを表すものなのでRとかGとか入っているが、それらはここでは無視してほしい。
    // ここでは 「32bit, 32bit, それぞれSigned(符号付)floatである」という意味を表すためだけにこれが指定されている。
    // ちなみにfloat型の3次元ベクトルを渡す際にはeR32G32B32Sfloatとか指定する。ここでもRGBの文字に深い意味はない。
    // 色のデータを渡すにしろ座標データを渡すにしろこういう名前の値を指定する。違和感があるかもしれないが、こういうものなので仕方ない。
    // offsetは頂点データのどの位置からデータを取り出すかを示す値。今回は1つしかアトリビュートが無いので0を指定しているが、複数のアトリビュートがある場合にはとても重要なものである。
    (*result).push_back(vk::VertexInputAttributeDescription());
    (*result).push_back(vk::VertexInputAttributeDescription());

    (*result)[0].binding = 0;
    (*result)[0].location = 0;
    (*result)[0].format = vk::Format::eR32G32Sfloat;
    (*result)[0].offset = offsetof(Vertex, pos);
    
    (*result)[1].binding = 0;
    (*result)[1].location = 1;
    (*result)[1].format = vk::Format::eR32G32B32Sfloat;
    (*result)[1].offset = offsetof(Vertex, color);

    return result;
}