<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

class CreateTrackQueueTrackTable extends Migration
{
    /**
     * Run the migrations.
     *
     * @return void
     */
    public function up()
    {
        Schema::create('track_queue_track', function (Blueprint $table) {
            $table->id();
            $table->unsignedBigInteger('track_queue_id')->default('');
            $table->unsignedBigInteger('track_id')->default('');
            $table->foreign('track_id')->references('id')->on('tracks')->onDelete('cascade');
            $table->foreign('track_queue_id')->references('id')->on('track_queues')->onDelete('cascade');
            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     *
     * @return void
     */
    public function down()
    {
        Schema::dropIfExists('track_queue_track');
    }
}
